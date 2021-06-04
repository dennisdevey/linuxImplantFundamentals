/*
 *  File: sash.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor client header file
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

#ifndef _SASH_H
#define _SASH_H

#include <termios.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "version.h"
#include "bfish.h"
#include "sadb.h"

/* Default name of config directory in users home directory */
#ifndef SASH_CONFIG_DIR
#define SASH_CONFIG_DIR		".sash"
#endif

/* Default name of sadb file in config directory  */
#ifndef SASH_SADB_FILE
#define SASH_SADB_FILE		"sash.db"
#endif

/* Default name of sash config file in config directory */
#ifndef SASH_CONFIG_FILE
#define SASH_CONFIG_FILE  "sash.conf"
#endif

/* Header to write to config file at creation */
#define SASH_CONFIG_FILE_HEADER \
	CFG_LINE_COMMENT"\n" \
	CFG_LINE_COMMENT" sash(1) " VERSION " configuration file\n" \
	CFG_LINE_COMMENT" See sash.conf(5) for more information.\n" \
	CFG_LINE_COMMENT"\n"

/* Default connection/listen timeout in seconds */
#define DEFAULT_CONN_TIMEOUT 30

struct sashopt {
	u_char *sa_home;      /* Path to config dir */
	u_char *sa_sadb;	  /* sadb file */
	u_char *sa_sacfg;     /* Config file */
	u_int sa_tip;		  /* Target IPv4 address */
	u_int sa_lip;         /* Local address */
	u_char sa_verbose;	  /* Verbose level */
	u_char sa_resolve:1;  /* Resolve hostname */
	time_t sa_ctout;      /* Connection timeout in seconds */
	time_t sa_tdelay;	  /* Delay in ms between packets */
	pid_t sa_pid;         /* PID of this process (for child) */
	u_char sa_esc;	      /* Escape character */

	/* Default packet values */
	u_long sa_randflags;   /* Random flags, see below */
	u_long sa_saddr;       /* Default source address */
	u_char sa_saddset:1;   /* Local address set in config file */
	u_char sa_iptos;       /* Default type of service */
	u_char sa_ipttl;       /* Default TTL */
	u_char sa_addttl;      /* Value to add to TTL */
	u_char sa_addttlset:1; /* Add to ttl above is set (avoid warning if zero) */
	u_short sa_ipid;       /* Default IP ID */
	u_short sa_udpsport;   /* Default UDP source port */
	u_short sa_udpdport;   /* Default UDP destination port */
	u_short sa_tcpsport;   /* Default TCP source port */
	u_short sa_tcpdport;   /* Default TCP destination port */
	u_long sa_tcpack;      /* Default ACK value */
	u_long sa_tcpseq;      /* Default SEQ number */
	u_char sa_tcpflags;    /* Default TCP flags */
	u_short sa_tcpwin;     /* Default TCP win */
	u_short sa_icmpid;     /* Default ICMP ID */
	u_short sa_icmpseq;    /* Default ICMP sequence number */
};

/* Random flags, do not edit.
 * These match flags in sashcfg.h */
#define RANDON_SRCIPV4		0x00020
#define RANOM_IPID          0x00080
#define RANDOM_IPTTL        0x00100
#define RANDOM_TCPACK		0x00400	
#define RANDOM_TCPSEQ		0x00800
#define RANDOM_UDP_SRCPORT	0x02000
#define RANDOM_UDP_DSTPORT	0x04000
#define RANDOM_TCP_SRCPORT	0x08000
#define RANDOM_TCP_DSTPORT	0x10000
#define RANDOM_ICMP_ID		0x20000
#define RANDOM_ICMP_SEQ		0x40000

/* Settings */
#define DEFAULT_IPTTL		64
#define DEFAULT_IPTOS		0x00
#define DEFAULT_TCPWIN   	512
#define DEFAULT_TCPFLAGS	(0x02 | 0x10)	/* SYN ACK */
#define DEFAULT_ICMPSEQ     0

#define DEFAULT_PACKET_DELAYMS 1
#define ESCAPE_PROMPT		"sash> "

/* CTRL(x) is not portable */
#define CONTROL(x) ((x)&0x1f)

/* Escape character */
#define ESCAPE_CHAR ']'

/* conn.c */
extern int conn_bindsock(long, u_short, struct sockaddr_in *);
extern int conn_accept(int, time_t);
extern int conn_connect(u_long, u_short);

/* connloop.c */
extern int connloop(int, struct bfish_key *);

/* escape.c */
extern u_char *escape_loop(int, struct termios *, int *);
extern void send_ftcmd(int, u_char, u_int, u_char *);

/* tty.c */
extern int tty_raw(int, struct termios *);

#endif /* _SASH_H */
