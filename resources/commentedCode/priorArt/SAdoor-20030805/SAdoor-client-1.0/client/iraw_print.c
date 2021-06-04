/*
 *  File: iraw_print.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Printing of raw IPv4 packets
 *  Version: 1.0
 *  Date: Mon Mar 17 20:11:03 CET 2003
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
#include "sash.h"
#include "net.h"
#include "iraw.h"


/* Global variables */
extern struct sashopt opt;


/*
 * Print packet to stream
 */
int
iraw_fprintpkt(FILE *stream, u_char *pkt)
{
	IPv4_hdr *iph = (IPv4_hdr *)pkt;
	TCP_hdr *tcph = NULL;
	UDP_hdr *udph = NULL;
	ICMP_hdr *ich = NULL;
	u_char *payload = NULL;
	u_char *pt;
	int paylen = 0;

	if (pkt == NULL) {
		fprintf(stderr, "** Warning: iraw_printpkt() "
			"Received NULL pointer as packet\n");
		return(-1);
	}

	if (iph->ip_prot == IP_PROTO_TCP) {
		tcph = (TCP_hdr *)(pkt + sizeof(IPv4_hdr));

		if ( (paylen = GETIPLENFIX(iph->ip_tlen) - 
				(4*iph->ip_hlen + 4*tcph->tcp_hlen)) > 0) 
			payload = (u_char *)((u_char *)tcph + 4*(tcph->tcp_hlen));

		if (opt.sa_verbose > 1)
			fprintf(stream, "(TCP) ");
	}

	else if (iph->ip_prot == IP_PROTO_UDP) {
		udph = (UDP_hdr *)(pkt + sizeof(IPv4_hdr));
	
		if ( (paylen = GETIPLENFIX(iph->ip_tlen) -
				(4*iph->ip_hlen + sizeof(UDP_hdr))) > 0) 
			payload = (u_char *)((u_char *)udph + sizeof(UDP_hdr));

		if (opt.sa_verbose > 1)
			fprintf(stream, "(UDP) ");
	}

	else if (iph->ip_prot == IP_PROTO_ICMP) {
		ich = (ICMP_hdr *)(pkt + sizeof(IPv4_hdr));

		if ( (paylen = GETIPLENFIX(iph->ip_tlen) -
				(4*iph->ip_hlen + sizeof(ICMP_hdr))) > 0) 
			payload = (u_char *)((u_char *)ich + sizeof(ICMP_hdr));

		if (opt.sa_verbose > 1) 
			fprintf(stream, "(ICMP) ");
	}

	else {
		fprintf(stream, "** Warning: Unrecognized protocol: 0x%02x\n", 
				iph->ip_prot);
		return(-1);
	}

	fprintf(stream, "%s", net_ntoa(iph->ip_sadd, NULL));

	if (tcph != NULL)
		fprintf(stream, ":%u", ntohs(tcph->tcp_sprt));
	else if (udph != NULL)
		fprintf(stream, ":%u", ntohs(udph->udp_sprt));

	if (opt.sa_resolve == 1) {
		if ( (pt = net_hostname2(iph->ip_sadd)) != NULL)
			fprintf(stream, " (%s)", net_hostname2(iph->ip_sadd));
	}
	
	fprintf(stream, " -> %s", net_ntoa(iph->ip_dadd, NULL));

    if (tcph != NULL)
        fprintf(stream, ":%u", ntohs(tcph->tcp_dprt));
    else if (udph != NULL)
        fprintf(stream, ":%u", ntohs(udph->udp_dprt));

	if (opt.sa_resolve == 1) {
		if ( (pt = net_hostname2(iph->ip_dadd)) != NULL)
			fprintf(stream, " (%s)", pt);
	}

	if (opt.sa_verbose > 2) {
		fprintf(stream, " tos=0x%02x id=0x%04x ttl=%u ", 
			iph->ip_tos, ntohs(iph->ip_id), iph->ip_ttl);
	}

	if ((opt.sa_verbose > 2) && (tcph != NULL)) {
		fprintf(stream, "seq=0x%08x ack=0x%08x flags=%s win=0x%04x ",
			ntohl(tcph->tcp_seq), ntohl(tcph->tcp_ack), 
			net_tcpflags_short(tcph->tcp_flgs), ntohs(tcph->tcp_win));
	}

	if ((opt.sa_verbose > 2) && (ich != NULL)) {
		fprintf(stream, "type=%d code=%d icmp_id=%d icmp_seq=%d ",
			ich->icmp_type, ich->icmp_code, ich->icmp_u32.icmp_echo.id, 
			ich->icmp_u32.icmp_echo.seq);
	}

	if ((opt.sa_verbose > 3) && (payload != NULL)) {
		int i = 0;

		printf("    ");
		for(; i<paylen; i++)
			fprintf(stream, "%c", BYTETABLE[payload[i]]);
	}
	fprintf(stream, "\n");
	fflush(stream);
	
	return(0);
}
