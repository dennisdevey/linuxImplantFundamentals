/*
 *  File: getconf.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod config code writing procedure(s).
 *  Version: 1.0
 *  Date: Tue Jun 24 18:30:25 CEST 2003
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "getconf.h"

#define KEYPKT_NAME            "keypkt"
#define CMDPKT_NAME            "cpkt"
#define PACKETS_NAME        "sapackets"
#define PACKETS_PT_NAME     "packets"
#define INCLUDE                "#include <sys/types.h>\n" \
                            "#include <sys/param.h>\n" \
                            "#include \"sapc.h\""

/* Local functions */
static void write_proto(FILE *, struct sa_pkt *, long long);

/*
 * Write packet configuration code for the kernel module.
 * Returns -1 on error, 0 on success.
 */
int
writecode(u_char *dstfile, struct sa_pkts *packets)
{
    struct sa_pkt *pkt;
    struct sa_pkt **pktlist;
    long long i = 0;
    FILE *out;

    if ( (out = fopen(dstfile, "w")) == NULL) {
        fprintf(stderr, "** Error: fopen(%s): %s\n", 
            dstfile, strerror(errno));
        return(-1);
    }


    /* Allocate an array to hold the packet-pointers.
     * We need this to write the packets in the reverse order */
    if ( (pktlist = (struct sa_pkt **)calloc(packets->sa_pkts_count-1, 
            sizeof(struct sa_pkt *))) == NULL) {
        fprintf(stderr, "** Error: calloc(): %s\n",
            strerror(errno));
        return(-1);
    }


    /* Set all packet pointers */
    pkt = packets->key_pkts;
    i = 0;
    for (; pkt!=NULL; pkt=pkt->sa_kpkt_next, i++) {
        pktlist[i] = pkt;
    }

    /* Write the required include files */
    fprintf(out, "%s\n\n", INCLUDE);

    /* Write all key packets, last first 
     * (packets->sa_pkts_count counts the command packet as well) */
    i = packets->sa_pkts_count -2;
    pkt = pktlist[i];
    while (i>=0) {
         pkt = pktlist[i];
        
        /* Write protocol header first */
        if (pkt->sa_pkt_ph != NULL) 
            write_proto(out, pkt, i);
    
        /* Key packet "Header" */
        fprintf(out, "/* Key packet %u */\n", (u_int)i);
        fprintf(out, "static struct sa_pkt %s%u = \n{\n", KEYPKT_NAME, (u_int)i);
        
        /* IP */
        fprintf(out, "\t0x%02x,\t\t/* Field flags */\n", pkt->sa_iph_fields);
        fprintf(out, "\t0x%08x,\t/* Destination address */\n", (u_int)pkt->sa_iph_daddr);
        fprintf(out, "\t0x%08x,\t/* Source address */\n", (u_int)pkt->sa_iph_saddr);
        fprintf(out, "\t0x%02x,\t\t/* Type of service */\n", pkt->sa_iph_tos);
        fprintf(out, "\t0x%04x,\t\t/* IP Identity */\n", pkt->sa_iph_id);
        fprintf(out, "\t0x%02x,\t\t/* Time To Live */\n", pkt->sa_iph_ttl);
        fprintf(out, "\t0x%04x,\t\t/* Protocol */\n", pkt->sa_pkt_proto);

        if (pkt->sa_pkt_ph != NULL)
            fprintf(out, "\t(void *)&%s%u_proto,\t/* Protocol pointer */\n", KEYPKT_NAME, (u_int)i);
        else
            fprintf(out, "\tNULL,\t/* No Protocol header */\n");
        
        if (pkt->sa_kpkt_next != NULL)
            fprintf(out, "\t&%s%u\t/* Next pointer */\n", KEYPKT_NAME, (u_int)i+1);
        else
            fprintf(out, "\tNULL\t\t/* No next pointer (Last packet) */\n");
        
        fprintf(out, "};\n\n");

        i--;
    }

    /* Write command packet */
    pkt = packets->cmd_pkt;
    
    if (pkt->sa_pkt_ph != NULL)
        write_proto(out, pkt, -1);

    fprintf(out, "/* Command packet */\n");
    fprintf(out, "static struct sa_pkt %s = \n{\n", CMDPKT_NAME);
    
    /* IP */
    fprintf(out, "\t0x%02x,\t\t/* Field flags */\n", pkt->sa_iph_fields);
    fprintf(out, "\t0x%08x,\t/* Destination address */\n", (u_int)pkt->sa_iph_daddr);
    fprintf(out, "\t0x%08x,\t/* Source address */\n", (u_int)pkt->sa_iph_saddr);
    fprintf(out, "\t0x%02x,\t\t/* Type of service */\n", pkt->sa_iph_tos);
    fprintf(out, "\t0x%04x,\t\t/* IP Identity */\n", pkt->sa_iph_id);
    fprintf(out, "\t0x%02x,\t\t/* Time To Live */\n", pkt->sa_iph_ttl);
    fprintf(out, "\t0x%04x,\t\t/* Protocol */\n", pkt->sa_pkt_proto);
                               
    if (pkt->sa_pkt_ph != NULL)
        fprintf(out, "\t(void *)&%s_proto,\t/* Protocol pointer */\n", CMDPKT_NAME);
    else
        fprintf(out, "\tNULL,\t/* No Protocol header */\n");
    fprintf(out, "\tNULL\t\t/* No next pointer in command packet */\n");

    fprintf(out, "};\n\n");
    

    /* Write packets structure */
    fprintf(out, "/* Packets structure */\n");
    fprintf(out, "static struct sa_pkts %s = \n{\n", PACKETS_NAME);
    fprintf(out, "\t0x%08x,\t/* Packet count */\n", packets->sa_pkts_count);
    fprintf(out, "\t&%s0,\t/* Key packets */\n", KEYPKT_NAME);
    fprintf(out, "\t&%s,\t\t/* Command packet */\n", CMDPKT_NAME);
    fprintf(out, "};\n\n");

    fprintf(out, "struct sa_pkts *%s = &%s;\n\n", PACKETS_PT_NAME, PACKETS_NAME);

    return(0);
}

/*
 * Write protocol header for current packet
 */
static void
write_proto(FILE *out, struct sa_pkt *pkt, long long index)
{
    u_char sname[48];
    u_short paylen = 0;
    u_char *payload = NULL;
    u_short i;
    
    if (index < 0) {
        fprintf(out, "/* Protocol header for command packet */\n");
        snprintf(sname, sizeof(sname), "%s_proto", CMDPKT_NAME);
    }
    else {
        fprintf(out, "/* Protocol header for key packet %u */\n", (u_int)index);
        snprintf(sname, sizeof(sname), "%s%u_proto", KEYPKT_NAME, (u_int)index);
    }
    
    /* TCP */
    if (pkt->sa_pkt_proto == SAPKT_PROTO_TCP) {
        struct sa_tcph *ptcp;
        
        ptcp = (struct sa_tcph *)pkt->sa_pkt_ph;
        fprintf(out, "static struct sa_tcph %s = \n{\n", sname);
        fprintf(out, "\t0x%02x,\t\t/* Field flags */\n", ptcp->sa_tcph_fields);
        fprintf(out, "\t0x%04x,\t\t/* Destination port */\n", ptcp->sa_tcph_dport);
        fprintf(out, "\t0x%04x,\t\t/* Source port */\n", ptcp->sa_tcph_sport);
        fprintf(out, "\t0x%08x,\t/* Sequence number */\n", (u_int)ptcp->sa_tcph_seq);
        fprintf(out, "\t0x%08x,\t/* Acknowledge number */\n", (u_int)ptcp->sa_tcph_ack);
        fprintf(out, "\t0x%02x,\t\t/* Control flags */\n", ptcp->sa_tcph_flags);
        fprintf(out, "\t0x%04x,\t\t/* Payload length */\n", ptcp->sa_tcph_dlen);
        paylen = ptcp->sa_tcph_dlen;
        payload = ptcp->sa_tcph_data;    
    }

    /* UDP */
    else if (pkt->sa_pkt_proto == SAPKT_PROTO_UDP) {
        struct sa_udph *pudp;

        pudp = (struct sa_udph *)pkt->sa_pkt_ph;
        fprintf(out, "static struct sa_udph %s = \n{\n", sname);
        fprintf(out, "\t0x%02x,\t\t/* Field flags */\n", pudp->sa_udph_fields);
        fprintf(out, "\t0x%04x,\t\t/* Destination port */\n", pudp->sa_udph_dport);
        fprintf(out, "\t0x%04x,\t\t/* Source port */\n", pudp->sa_udph_sport);
        fprintf(out, "\t0x%04x,\t\t/* Payload length */\n", pudp->sa_udph_dlen);
        paylen = pudp->sa_udph_dlen;
        payload = pudp->sa_udph_data;
    }

    /* ICMP */
    else if (pkt->sa_pkt_proto == SAPKT_PROTO_ICMP) {
        struct sa_icmph *picmp;
        
        picmp = (struct sa_icmph *)pkt->sa_pkt_ph;
        fprintf(out, "static struct sa_icmph %s = \n{\n", sname);
        fprintf(out, "\t0x%02x,\t\t/* Field flags */\n", picmp->sa_icmph_fields);
        fprintf(out, "\t0x%02x,\t\t/* Code */\n", picmp->sa_icmph_code);
        fprintf(out, "\t0x%02x,\t\t/* Type */\n", picmp->sa_icmph_type);
        fprintf(out, "\t0x%04x,\t\t/* Identity */\n", picmp->sa_icmph_id);
        fprintf(out, "\t0x%04x,\t\t/* Sequence number */\n", picmp->sa_icmph_seq);
        fprintf(out, "\t0x%04x,\t\t/* Payload length */\n", picmp->sa_icmph_dlen);
        paylen = picmp->sa_icmph_dlen;
        payload = picmp->sa_icmph_data;
    }
    
    else {
        fprintf(stderr, "** Error: Unrecognized protocol (%u)\n", pkt->sa_pkt_proto);
        exit(EXIT_FAILURE);
    }

    /* Payload */
    if (payload == NULL)
        fprintf(out, "\tNULL\t\t/* No Payload */\n");
    else {
        fprintf(out, "\t\"");
        for (i=0; i<paylen; i++)
            fprintf(out, "\\x%02x", payload[i]);
        fprintf(out, "\" /* Payload */\n");
    }

    fprintf(out, "};\n\n");
}

