/*
 *  File: sadb.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor database header file
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

#ifndef _SADB_H
#define _SADB_H

#include <sys/types.h>
#include "sapc.h"
#include "utils.h"

/* Align data to this value */
#define SADB_ALIGN    4

/* Returns the value needed to align x to num */
#define SA_ALIGN(x, num) (((x) % (num)) ? (num) - ((x) % (num)) : 0)

/* File header */
struct safhdr {
	uint8_t iv[8];	 /* The IV used for this file */
	uint8_t text[8]; /* Used for key-check */
};
#define SADB_KEYCHECK		"SAdoor4"

/* SADB header */
struct sadbhdr {
    uint32_t sah_totlen;      /* Total length of entry in bytes */
    uint32_t sah_ipv4addr;    /* IPv4 address in network byte order */
	uint8_t sah_bfkey[56];    /* Blowfish key */
    uint8_t sah_ver[24];      /* SAdoor version */
    uint32_t sah_btime;       /* Time of build for this entry */
    struct sahost {
        uint8_t sah_os[12];   /* Name of this OS */
        uint8_t sah_rel[12];  /* Release level */
        uint8_t sah_mah[12];  /* Hardware type */
    } host;
    uint8_t sah_flags;        /* Flags, se below */
    uint32_t sah_tout;        /* Command packet timeout */
    uint32_t sah_pktcount;    /* Number of packets */
};

struct sadbent {
    struct sadbhdr *se_hdr;
    struct sa_pkt *se_kpkts;   /* Linked list of key packets */
    struct sa_pkt *se_cpkt;    /* Command packet */
};

#define SADOOR_PROMISC_FLAG     0x1
#define PROMISC(x)        ((x) & SADOOR_PROMISC_FLAG)
#define SADOOR_NOSINGLE_FLAG    0x2
#define NOSINGLE(x)        ((x) & SADOOR_NOSINGLE_FLAG)
#define SADOOR_NOACCEPT_FLAG    0x4
#define NOACCEPT(x)     ((x) & SADOOR_NOACCEPT_FLAG)
#define SADOOR_NOCONNECT_FLAG   0x8
#define NOCONNECT(x)     ((x) & SADOOR_NOCONNECT_FLAG)


/* sadb.c */
extern struct sadbent *sadb_getentip(const struct mfile *, uint32_t, struct sadbent *);
extern struct sadbent *sadb_genent(uint8_t *, uint32_t, struct sadbent *);
extern int sadb_writeraw(int, struct sadbent *, int);
extern void sadb_printent(struct sadbent *, int, u_char, u_char);

#endif /* _SADB_H */
