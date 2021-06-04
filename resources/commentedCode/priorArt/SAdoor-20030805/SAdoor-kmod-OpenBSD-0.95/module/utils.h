/*
 *  File: utils.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod common used routines header file
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

#ifndef _UTILS_H
#define _UTILS_H

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include "sadoor.h"

struct sys_write_args;
struct sys_read_args;

/* utils.c */
extern void *uland_calloc(size_t);
extern void uland_free(void *);
extern void *kspace_calloc(size_t);
extern int fork_from_init(void (*)(void *), void *);
extern void do_system(void *);
extern ssize_t writen(struct proc *, struct sys_write_args *);
extern ssize_t readn(struct proc *, struct sys_read_args *);
extern ssize_t utils_memstr(void *, size_t, const void *, size_t);
extern int ready_write(int);

/* Calls /bin/sh -c command from userspace */
#define kernel_system(command) fork_from_init(do_system, (void *)(command))

/* Free kernel space */
#define kspace_free(kpt) free((kpt), M_TEMP)

#endif /* _UTILS_H */
