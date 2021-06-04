/*
 *  File: utils.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Common used routines header file.
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

#ifndef _UTILS_H
#define _UTILS_H

/* Returns 1 if strings match and is not NULL */
#define MATCH(__s1, __s2) \
    ((((__s1) != NULL) && ((__s2) != NULL)) ? (strcmp((__s1), (__s2)) == 0) : 0)

/* Returns 1 if strings match (ignores case) and is not NULL */
#define CMATCH(__s1, __s2) \
    ((((__s1) != NULL) && ((__s2) != NULL)) ? (strcasecmp((__s1), (__s2)) == 0) : 0)

#define ISu6(x) (((x) >= 0) && ((x) <= 0x3f))
#define ISu8(x) (((x) >= 0) && ((x) <= 0xff))
#define ISu16(x) (((x) >= 0) && ((x) <= 0xffff))

#define ISHEX(c) ((((c) >= 'a') && ((c) <= 'f')) || \
                 (((c) >= 'A') && ((c) <= 'F')) || \
                 (((c) >= '0') && ((c) <= '9')))

/* utils.c */
extern u_char *strndup(u_char *, const size_t);
extern int strisnum(u_char *, long *);
extern int isip(u_char *);

#endif /* _UTILS_H */
