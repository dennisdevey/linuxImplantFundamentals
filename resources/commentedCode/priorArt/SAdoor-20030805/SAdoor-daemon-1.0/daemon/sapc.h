/*
 *  File: sapc.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor packet config header file.
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

#ifndef _SAPC_H
#define _SAPC_H

#include <netinet/in.h>
#include <sys/types.h>

/* Special (single-char) deliminators */
#define LINE_COMMENT    "#"
#define BLOCK_BEGIN     "{"
#define BLOCK_END       "}"
#define CMD_END         ";"
#define REPL_BEGIN      "("
#define REPL_END        ")"
#define ASSIGN          "="
#define SPEC_DELIMS     LINE_COMMENT BLOCK_BEGIN BLOCK_END CMD_END REPL_BEGIN REPL_END ASSIGN

#define IS_DELIM(c)      (isspace((int)(c)) || IS_SPEC_DELIM(c))
#define IS_SPEC_DELIM(c) ((c) != '\0' && strchr(SPEC_DELIMS, (c)) != NULL)

#define KEYPKT_TOK      "keypkt"
#define CMDPKT_TOK      "cmdpkt"

/* IP tokens */
#define IP_TOK          "ip"
#define IP_SADDR_TOK    "saddr"
#define IP_DADDR_TOK    "daddr"
#define IP_TOS_TOK      "tos"
#define IP_ID_TOK       "id"
#define IP_TTL_TOK      "ttl"

/* TCP tokens */
#define TCP_TOK          "tcp"
#define TCP_DPORT_TOK    "dport"
#define TCP_SPORT_TOK    "sport"
#define TCP_SEQ_TOK      "seq"
#define TCP_ACK_TOK      "ack"
#define TCP_FLAGS_TOK    "flags"

/* UDP tokens */
#define UDP_TOK          "udp"
#define UDP_DPORT_TOK    "dport"
#define UDP_SPORT_TOK    "sport"

/* ICMP tokens */
#define ICMP_TOK         "icmp"
#define ICMP_CODE_TOK    "code"
#define ICMP_TYPE_TOK    "type"
#define ICMP_ID_TOK      "id"
#define ICMP_SEQ_TOK     "seq"

#define DATA_TOK         "data"

#define FIELD_IS_SET(__x, __field) ((__x) & (__field))

/* TCP header */
#define TCPH_DPORT          0x01
#define TCPH_SPORT          0x02
#define TCPH_SEQ            0x04
#define TCPH_ACK            0x08
#define TCPH_FLAGS          0x10

struct sa_tcph {
    uint8_t sa_tcph_fields;
    uint16_t sa_tcph_dport; 
    uint16_t sa_tcph_sport;
    u_long sa_tcph_seq;
    u_long sa_tcph_ack;
    uint8_t sa_tcph_flags;
    uint16_t sa_tcph_dlen; /* Data length */
    uint8_t *sa_tcph_data;
};

/* UDP header */
#define UDPH_DPORT        0x01
#define UDPH_SPORT        0x02

struct sa_udph {
    uint8_t sa_udph_fields;
    uint16_t sa_udph_dport; 
    uint16_t sa_udph_sport;
    uint16_t sa_udph_dlen;    /* Data length */
    uint8_t *sa_udph_data;
};

/* ICMP header */
#define ICMPH_CODE         0x01
#define ICMPH_TYPE         0x02
#define ICMPH_ID           0x04
#define ICMPH_SEQ          0x08

/* Supported types */
#define IS_SUPPICMP_TYPE(x) ((x) == 0 || (x) == 8)

struct sa_icmph {
    uint8_t sa_icmph_fields;
    uint8_t sa_icmph_code;
    uint8_t sa_icmph_type;
	uint16_t sa_icmph_id;     
	uint16_t sa_icmph_seq;
    uint16_t sa_icmph_dlen;    /* Data length */
    uint8_t *sa_icmph_data;
};

/* IP protocol */
#define SAPKT_PROTO_TCP     0x06
#define SAPKT_PROTO_UDP     0x11
#define SAPKT_PROTO_ICMP    0x01
#define SAPKT_PROTO(x)    (((x) == NULL) ? 0 : (x)->sa_pkt_proto)

/* IP header fields */
#define IPH_DADDR       0x01
#define IPH_SADDR       0x02
#define IPH_TOS         0x04
#define IPH_ID          0x08
#define IPH_TTL         0x10

struct sa_pkt {
    uint8_t sa_iph_fields;
    u_long sa_iph_daddr;
    u_long sa_iph_saddr;
    uint8_t sa_iph_tos;
    uint16_t sa_iph_id;
    uint8_t sa_iph_ttl;

    uint16_t sa_pkt_proto;   /* sa_pkt_ph flags, SAPKT_PROTO_TCP ... */
    void *sa_pkt_ph;

    struct sa_pkt *sa_kpkt_next;
};

struct sa_pkts {
    uint32_t sa_pkts_count;
    struct sa_pkt *key_pkts;
    struct sa_pkt *cmd_pkt;    
};

/* sapc_lexer.c */
extern uint8_t **sapc_lexer(uint8_t *);

/* sapc_parser.c */
extern struct sa_pkts *sapc_getpkts(uint8_t *);
extern void free_pkts(struct sa_pkts *);

#endif /* _SAPC_H */
