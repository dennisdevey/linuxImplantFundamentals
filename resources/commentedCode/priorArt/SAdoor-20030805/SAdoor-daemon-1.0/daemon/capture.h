/*
 *  File: capture.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor IP sniff structures and definitions.
 *  Version: 1.0
 *  Date: Tue Jan  7 23:24:15 CET 2003
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

#ifndef _CAPTURE_H
#define _CAPTURE_H

#include <netinet/in.h>
#include <sys/types.h>
#include <pcap.h>

/* Capture whole packet (most MTU is 1500) */
#define CAP_SNAPLEN        1500
#define CAP_TIMEOUT        1000

/*
 * "Need to know" when using the capture functions
 */
struct capture {
    pcap_t *cap_pcapd;       /* Pcap descriptor */
    int cap_offst;           /* Link layer offset */
    bpf_u_int32 cap_net;     /* Local network address */
    bpf_u_int32 cap_mask;    /* Netmask of local network */
};

#define PROTO_TCP        0x06    
#define PROTO_UDP        0x11
#define PROTO_ICMP        0x1

/*
 * Internet Protocol version 4 header
 */
typedef struct {

#ifdef WORDS_BIGENDIAN
    uint8_t ip_ver :4,    /* IP version */
    ip_hlen: 4;           /* Header length in (4 byte) words */
#else
    uint8_t ip_hlen :4,   /* Header length in (4 byte) words */
    ip_ver: 4;            /* IP version */
#endif
    uint8_t ip_tos;       /* Type of service */
    uint16_t ip_tlen;     /* Datagram total length */
    uint16_t ip_id;       /* Identification number */
#ifdef WORDS_BIGENDIAN
    uint16_t ip_flgs: 3,  /* Fragmentation flags */
    ip_off: 13;           /* Fragment offset */
#else
    uint16_t ip_off: 13,  /* Fragment offset */
    ip_flgs: 3;           /* Fragmentation flags */
#endif
    uint8_t ip_ttl;       /* Time to live */
    uint8_t ip_prot;      /* Transport layer protocol (ICMP=1, TCP=6, UDP=17) */
    uint16_t ip_sum;      /* Checksum */
    uint32_t ip_sadd;     /* Source address */
    uint32_t ip_dadd;     /* Destination address */
} IPv4_hdr;

/*
 * Transmission Control Protocol header
 */
typedef struct {
    uint16_t tcp_sprt;    /* Source port */
    uint16_t tcp_dprt;    /* Destination port */
    uint32_t tcp_seq;     /* Sequence number */
    uint32_t tcp_ack;     /* Acknowledgement number */
#ifdef WORDS_BIGENDIAN
    uint8_t tcp_hlen: 4,  /* Header length */
    tcp_zero: 4;          /* Unused, should be zero */
#else
    uint8_t tcp_zero: 4,  /* Unused, should be zero */
    tcp_hlen: 4;          /* Header length */
#endif
    uint8_t tcp_flgs;     /* 6 bit control flags, see below */
    uint16_t tcp_win;     /* Size of sliding window */
    uint16_t tcp_sum;     /* Checksum */
    uint16_t tcp_urg;     /* Urgent pointer (if URG flag is set) */
} TCP_hdr;

/*
 * User Datagram Protocol Header
 */
typedef struct {
    uint16_t udp_sprt;    /* Source port */
    uint16_t udp_dprt;    /* Destination port */
    uint16_t udp_len;     /* Length of UDP header including data */
    uint16_t udp_sum;     /* Checksum */
} UDP_hdr;

/*
 * Internet Control Message Protocol Header
 */
typedef struct {
    uint8_t icmp_type;
    uint8_t icmp_code; 
    uint16_t icmp_sum;
	union {
		struct {
			uint16_t id;  
			uint16_t seq; 
		} icmp_echo;
		uint32_t gw;	/* gateway if type is 5 */
	} icmp_u32;
} ICMP_hdr;


/*
 * Checksum header
 * Used for UDP and TCP checksum calculations.
 * W. Richard stevens TCP/IP illustrated Vol 1 page 145.
 */
typedef struct {
    uint32_t pse_sadd;   /* Source address */
    uint32_t pse_dadd;   /* Destination address */
    uint8_t pse_zero;    /* Zero byte */
    uint8_t pse_proto;   /* Protocol code */
    uint16_t pse_hlen;   /* Length of TCP/UDP header */
} Psuedo_hdr;


/* capture.c */
extern struct capture *cap_open(uint8_t *, int);
extern int cap_setfilter(struct capture *, uint8_t *);
extern int cap_packets(struct capture *, pcap_handler);

#endif /* _CAPTURE_H */
