/*
 *  File: utils.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Common used routines header file.
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

#ifndef _UTILS_H
#define _UTILS_H

#include <sys/types.h>
#include <sys/ioctl.h>

/* Key types for getkey() */
#define NEW_KEY			0
#define FILE_KEY		1

/* Memory mapped file */
struct mfile {
	int mf_fd;			/* File descriptor */
	size_t mf_size;		/* Size of file in bytes */
	u_char *mf_file;	/* Pointer to start of file */
};

/* utils.c */
extern int isip(u_char *);
extern struct mfile *open_mfile(u_char *);
extern int close_mfile(struct mfile *);
extern u_char *timestr(time_t, u_char *);
extern ssize_t writen(int, const void *, size_t);
extern ssize_t readn(int, const void *, size_t);
extern ssize_t memstr(const void *, size_t, const void *, size_t);
extern int ready_write(int);
extern u_char *strndup(u_char *, const size_t);
extern int strisnum(u_char *, long *);
extern u_char *getkey(u_char, const u_char *);

#endif /* _UTILS_H */
