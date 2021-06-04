/*
 *  File: iraw.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Raw IPv4 packet routines
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
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sash.h"
#include "iraw.h"

/* Local procedures */
static u_short chksum(u_short *, int);

/* Global variables */
extern struct sashopt opt;

/*
 * Generates header checksum.
 * W. Richard Stevens TCP/IP illustrated vol. 1 page 145
 */
u_short
chksum(uint16_t *buf, int nwords)
{
    u_long sum = 0;

    for(; nwords > 0; nwords--)
        sum += *buf++;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return(~sum);
}

/*
 * Add IPv4 header to buffer buf.
 * The total length field as well as the IP checksum 
 * is set when a protocol is added.
 * Note that all values is assumed to be in network byte order.
 * Returns -1 on error and zero on success.
 */
int
iraw_add_ipv4(uint8_t *buf, uint8_t tos, uint16_t id, 
		uint8_t ttl, uint32_t sadd, uint32_t dadd)
{
	IPv4_hdr *iph;

	if (buf == NULL) {
		fprintf(stderr, "** Error: iraw_add_ipv4(): "
			"Received NULL pointer as buf\n");
		return(-1);
	}

	iph = (IPv4_hdr *)buf;
	iph->ip_ver = 4;
	iph->ip_hlen = 5;
	iph->ip_tos = tos;
	iph->ip_id = id;
	iph->ip_flgs = 0x0;	/* (Don't) fragment */
	iph->ip_off = 0x00; /* Fragment offset */
	iph->ip_ttl = ttl;
	iph->ip_sadd = sadd;
	iph->ip_dadd = dadd;

	return(0);	
}

/*
 * A TCP header is added to the buffer which is assumed to already
 * contain an IPv4 header. The total length and proto field in the IPv4 
 * header is set here as well as the TCP and IP checksum.
 * Note that all values is assumed to be in network byte order (except paylen).
 * Returns -1 on error and zero on success.
 */
int
iraw_add_tcp(uint8_t *packet, uint16_t sport, uint16_t dport, uint32_t seq, 
		uint32_t ack, uint8_t flags, uint8_t *payload, uint16_t paylen)
{
	IPv4_hdr *iph;
	TCP_hdr *tcph;
	Pseudo_hdr *phdr;
	uint8_t pbuf[sizeof(Pseudo_hdr) + sizeof(TCP_hdr) + paylen + (paylen % 2)];

	memset(pbuf, 0x00, sizeof(pbuf));
	iph = (IPv4_hdr *)packet;	

	if (packet == NULL) {
		fprintf(stderr, "** Error: iraw_add_tcp(): "
			"Received NULL pointer as packet\n");
		return(-1);
	}

	if ((paylen + sizeof(IPv4_hdr) + sizeof(TCP_hdr)) > PACKET_MAX_SIZE) {
		fprintf(stderr, "** Error: Packet (%u bytes) is greater than "
			"maximum size allowed (%u bytes)\n", (paylen + sizeof(IPv4_hdr) + 
			sizeof(TCP_hdr)), PACKET_MAX_SIZE);
		return(-1);
	}

	/* Build Pseudo header  */
	phdr = (Pseudo_hdr *)pbuf;
	phdr->phd_saddr = iph->ip_sadd;
	phdr->phd_daddr = iph->ip_dadd;
	phdr->phd_zero = 0;
	phdr->phd_proto = IP_PROTO_TCP;
	phdr->phd_hlen = htons(sizeof(TCP_hdr) + paylen);

	/* Build TCP header */
	tcph = (TCP_hdr *)(pbuf + sizeof(Pseudo_hdr));
	tcph->tcp_sprt = sport;
	tcph->tcp_dprt = dport;
	tcph->tcp_seq = seq;
	tcph->tcp_ack = ack;
	tcph->tcp_hlen = 5;
	tcph->tcp_zero = 0;
	tcph->tcp_flgs = flags;
	tcph->tcp_win = opt.sa_tcpwin;
	tcph->tcp_urg = 0;
	memcpy(pbuf + sizeof(Pseudo_hdr) + sizeof(TCP_hdr), payload, paylen);	

	/* TCP checksum */
#ifdef STUPID_SOLARIS_CHECKSUM_BUG
	tcph->tcp_sum = sizeof(TCP_hdr) + paylen;
#else
	tcph->tcp_sum = chksum((u_short *)pbuf, (sizeof(Pseudo_hdr) +
		sizeof(TCP_hdr) + paylen + (paylen % 2)) >> 1);
#endif

	/* Copy TCP header to real packet */
	memcpy(packet + sizeof(IPv4_hdr), pbuf + sizeof(Pseudo_hdr), sizeof(TCP_hdr) + paylen);

    /* Set remaining IP header values and calculate IP checksum */
    iph = (IPv4_hdr *)packet;
    iph->ip_tlen = SETIPLENFIX(sizeof(IPv4_hdr) + sizeof(TCP_hdr) + paylen);
    iph->ip_prot = IP_PROTO_TCP;
    iph->ip_sum = chksum((u_short *)packet, (GETIPLENFIX(iph->ip_tlen) +  (paylen % 2))>> 1);
	
	return(0);	
}

/*
 * A UDP header is added to the buffer which is assumed to already 
 * contain an IPv4 header. The total length and proto field in the IPv4
 * header is set here as well as the checksum.
 * We also calculate the UDP checksum each time. This could perhaps be
 * randomized since it is optional under IPv4.
 * All arguments (except paylen) assumed to be in network byte order.
 */
int
iraw_add_udp(uint8_t *packet, uint16_t sport, uint16_t dport, 
			 uint8_t *payload, uint16_t paylen)
{
	IPv4_hdr *iph;
	UDP_hdr *udph;
	Pseudo_hdr *phdr;
    uint8_t pbuf[sizeof(Pseudo_hdr) + sizeof(UDP_hdr) + paylen + (paylen % 2)];
	
    memset(pbuf, 0x00, sizeof(pbuf));
    iph = (IPv4_hdr *)packet;

    if (packet == NULL) {
        fprintf(stderr, "** Error: iraw_add_tcp(): "
            "Received NULL pointer as packet\n");
        return(-1);
    }

    if ((paylen + sizeof(IPv4_hdr) + sizeof(UDP_hdr)) > PACKET_MAX_SIZE) {
        fprintf(stderr, "** Error: Packet (%u bytes) is greater than "
            "maximum size allowed (%u bytes)\n", (paylen + sizeof(IPv4_hdr) +
            sizeof(UDP_hdr)), PACKET_MAX_SIZE);
        return(-1);
    }

    /* Build Pseudo header  */
    phdr = (Pseudo_hdr *)pbuf;
    phdr->phd_saddr = iph->ip_sadd;
    phdr->phd_daddr = iph->ip_dadd;
    phdr->phd_zero = 0;
    phdr->phd_proto = IP_PROTO_UDP;
    phdr->phd_hlen = htons(sizeof(UDP_hdr) + paylen);

    /* Build UDP header */
    udph = (UDP_hdr *)(pbuf + sizeof(Pseudo_hdr));
    udph->udp_sprt = sport;
    udph->udp_dprt = dport;
    udph->udp_len = htons(sizeof(UDP_hdr)+paylen);
    memcpy(pbuf + sizeof(Pseudo_hdr) + sizeof(UDP_hdr), payload, paylen);

    /* UDP checksum */
#ifdef STUPID_SOLARIS_CHECKSUM_BUG
    udph->udp_sum = sizeof(UDP_hdr) + paylen;
#else
    udph->udp_sum = chksum((u_short *)pbuf, (sizeof(Pseudo_hdr) +
        sizeof(UDP_hdr) + paylen + (paylen % 2)) >> 1);
#endif

    /* Copy UDP header to real packet */
    memcpy(packet + sizeof(IPv4_hdr), pbuf + sizeof(Pseudo_hdr), 
		sizeof(UDP_hdr) + paylen);

    /* Set remaining IP header values and calculate IP checksum */
    iph = (IPv4_hdr *)packet;
    iph->ip_tlen = SETIPLENFIX(sizeof(IPv4_hdr) + sizeof(UDP_hdr) + paylen);
    iph->ip_prot = IP_PROTO_UDP;
    iph->ip_sum = chksum((u_short *)packet, 
		(GETIPLENFIX(iph->ip_tlen) +  (paylen % 2))>> 1);

	return(0);
}

/*
 * An ICMP header (bah, only type 0 or 8 ..) is added to the buffer 
 * which is assumed to already contain an IP header.
 * The total length and proto field in the IPv4 header is set here 
 * as well as the checksum.
 * All arguments (except paylen) assumed to be in network byte order.
 * Returns 0 on error and -1 on success.
 */
int
iraw_add_icmp(uint8_t *packet, uint8_t type, uint8_t code, 
	uint16_t id, uint16_t seq, uint8_t *payload, uint32_t paylen)
{
	IPv4_hdr *iph;
	ICMP_hdr *ich;

	iph = (IPv4_hdr *)packet;

	if (packet == NULL) {
		fprintf(stderr, "** Error: iraw_add_icmp(): "
			"Received NULL pointer as packet\n");
		return(-1);
	}

	if ((paylen + sizeof(IPv4_hdr) + sizeof(ICMP_hdr) + (paylen % 2)) > PACKET_MAX_SIZE) {
		fprintf(stderr, "** Error: Packet (%u bytes) is greater than "
			"maximum size allowed (%u bytes)\n", (paylen + sizeof(IPv4_hdr) +
			sizeof(ICMP_hdr)), PACKET_MAX_SIZE);
		return(-1);
	}

	/* Build ICMP */
	ich = (ICMP_hdr *)(packet + sizeof(IPv4_hdr));
	ich->icmp_type = type;
	ich->icmp_code = code;
	ich->icmp_sum = 0;
	ich->icmp_u32.icmp_echo.id = id;
	ich->icmp_u32.icmp_echo.seq = seq;
	memcpy((uint8_t *)(packet + sizeof(IPv4_hdr) + sizeof(ICMP_hdr)), payload, paylen);
	
	
	/* ICMP checksum */
#ifdef STUPID_SOLARIS_CHECKSUM_BUG
	ich->icmp_sum = sizeof(ICMP_hdr) + paylen;
#else
	ich->icmp_sum = chksum((u_short *)ich, (sizeof(ICMP_hdr)+paylen + (paylen % 2)) >> 1);
#endif

	/* Set remaining IP header values and calculate IP checksum */
	iph->ip_tlen = SETIPLENFIX(sizeof(IPv4_hdr) + sizeof(ICMP_hdr) + paylen);
	iph->ip_prot = IP_PROTO_ICMP;
	iph->ip_sum = chksum((u_short *)packet, 
		(GETIPLENFIX(iph->ip_tlen) +  (paylen % 2))>> 1);
	
	return(0);
}

/*
 * Send the packet.
 * Returns zero on success and -1 on error.
 * If working as a Chef was 5 out of 10,
 * this is 100 out of 1.
 */
int
iraw_send_packet(int raw_sock, uint8_t *packet)
{
	struct sockaddr_in sin;

	/* Create IPv4 address */
	memset(&sin, 0x00, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ((IPv4_hdr *)packet)->ip_dadd;

	/* Set destination port */
	if (((IPv4_hdr *)packet)->ip_prot == IP_PROTO_TCP)
		sin.sin_port = ((TCP_hdr *)(packet + sizeof(IPv4_hdr)))->tcp_dprt;
	else if (((IPv4_hdr *)packet)->ip_prot == IP_PROTO_UDP)
		sin.sin_port = ((UDP_hdr *)(packet + sizeof(IPv4_hdr)))->udp_dprt;

	/* Tell kernel that our datagram include headers */
	{
		int one = 1;
		const int *val = &one;
		if (setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, 
				val, sizeof(one)) < 0) {
			fprintf(stderr, "** Warning: iraw_send_packet(): "
				"Cannot set header included: %s\n",
				strerror(errno));
		}
	}

	/* Send packet */
	if (sendto(raw_sock, packet, 
			GETIPLENFIX(((IPv4_hdr *)packet)->ip_tlen), 0, 
			(struct sockaddr *)&sin, sizeof(sin)) < 0) {

		fprintf(stderr, "** Error: sendto(%d, %p, %d, 0, "
				"%p, %d): %s\n", raw_sock, packet, 
				GETIPLENFIX(((IPv4_hdr *)packet)->ip_tlen), 
				&sin, sizeof(sin), strerror(errno));

		fprintf(stderr, "BAD PACKET: ");
		iraw_fprintpkt(stderr, packet);
		return(-1);
	}
	
	if (opt.sa_verbose)
		iraw_printpkt(packet);

	return(0);
}
