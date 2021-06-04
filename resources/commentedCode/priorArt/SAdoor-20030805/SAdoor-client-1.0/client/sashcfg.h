/*
 *  File: sashcfg.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor client configuration header file
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

#ifndef _SASHCFG_H
#define _SASHCFG_H

#include <sys/types.h>
#include "sash.h"

/* Returns 1 if strings match (ignores case) and is not NULL */
#define CMATCH(__s1, __s2) \
    ((((__s1) != NULL) && ((__s2) != NULL)) ? (strcasecmp((__s1), (__s2)) == 0) : 0)

#define ISu8(x) (((x) >= 0) && ((x) <= 0xff))
#define ISu16(x) (((x) >= 0) && ((x) <= 0xffff))
#define ISu6(x) (((x) >= 0) && ((x) <= 0x3f))


/* Special (single-char) deliminators */
#define CFG_LINE_COMMENT    "#"
#define CFG_BLOCK_BEGIN     "{"
#define CFG_BLOCK_END       "}"
#define CFG_CMD_END         ";"
#define CFG_ASSIGN          "="
#define CFG_SPEC_DELIMS     CFG_LINE_COMMENT CFG_BLOCK_BEGIN CFG_BLOCK_END CFG_CMD_END CFG_ASSIGN

#define IS_CFG_DELIM(c)      (isspace((int)(c)) || IS_SPEC_CFG_DELIM(c))
#define IS_SPEC_CFG_DELIM(c) ((c) != '\0' && strchr(CFG_SPEC_DELIMS, (c)) != NULL)

/* Tokens, case ignored */
#define RANDOM_TOK      "random"
#define BASEADDR_TOK	"baseaddr"
#define SADB_TOK        "sadb"
#define TIMEOUT_TOK     "timeoutsec"
#define DELAYMS_TOK     "delayms"
#define ESCCHR_TOK      "escchar"
#define SADDR_TOK       "srcaddr"
#define IPTOS_TOK       "iptos"
#define IPID_TOK        "ipid"
#define IPTTL_TOK       "ttl"
#define ADDTTL_TOK      "addtottl"
#define TCPACK_TOK      "tcpack"
#define TCPSEQ_TOK      "tcpseq"
#define TCPFLAGS_TOK    "tcpflags"
#define UDPSPORT_TOK    "udpsrcport"
#define UDPDPORT_TOK    "udpdstport"
#define TCPSPORT_TOK    "tcpsrcport"
#define TCPDPORT_TOK    "tcpdstport"
#define TCPWIN_TOK      "tcpwin"
#define ICMPID_TOK		"icmpid"
#define ICMPSEQ_TOK		"icmseq"

/* Entry flags, used by parser to mark that
 * a value has been set for that field.
 * Random flags in sash.h match these.
 * DO NOT EDIT! */
#define BASEADDR_FLAG		0x00001
#define SADB_FLAG			0x00002
#define TIMEOUT_FLAG		0x00004
#define DELAYMS_FLAG		0x00008
#define ESCCHR_FLAG			0x00010
#define SADDR_FLAG			0x00020
#define IPTOS_FLAG			0x00040
#define IPID_FLAG			0x00080
#define IPTTL_FLAG			0x00100
#define ADDTTL_FLAG			0x00200
#define TCPACK_FLAG			0x00400
#define TCPSEQ_FLAG			0x00800
#define TCPFLAGS_FLAG		0x01000
#define UDPSPORT_FLAG		0x02000
#define UDPDPORT_FLAG		0x04000
#define TCPSPORT_FLAG		0x08000
#define TCPDPORT_FLAG		0x10000
#define TCPWIN_FLAG			0x80000
#define ICMPID_FLAG			0x20000
#define ICMPSEQ_FLAG		0x40000

/* sashcfg.c */
extern int sashcfg_setopt(struct sashopt *);


#endif /* _SASHCFG_H */
