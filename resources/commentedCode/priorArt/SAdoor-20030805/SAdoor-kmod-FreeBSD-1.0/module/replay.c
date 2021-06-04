/*
 *  File: replay.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod replay protection routines
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include "sadoor.h"

#ifdef SAKMOD_ENABLE_REPLAY_PROTECTION

/* Private variables */
static size_t elems = 0;
static struct rplog *first = NULL;
static struct rplog *last = NULL;

/*
 * Check timestamp against previous.
 * Returns 1 if ok, 0 otherwise.
 */
int
replay_check(long int sec, long int usec)
{
    struct rplog *tmp;


    if (sec > last->rp_tv.tv_sec)
        return(1);

    if (sec == last->rp_tv.tv_sec) 
        if (usec > last->rp_tv.tv_usec)
            return(1);

    /* Possible replay attack/clock skew
     * Check for this timestamp among the logged ones, if we find 
     * a match there is a replay attack on us. 
     * It the timestamp is smaller than the first one, we don't know
     * if it's a clock skew or a replay attack */

    /* Search for match */
    for (tmp = first; tmp != NULL; tmp = tmp->rp_next) {
        if ((tmp->rp_tv.tv_sec == sec) && (tmp->rp_tv.tv_usec == usec))
            break;
    }

    if (tmp != NULL) {
        log(2, "Alert: Replay Attack detected, found received timestamp "
            "(sec: 0x%08x usec: 0x%08x) among logged\n", 
            (u_int)tmp->rp_tv.tv_sec, (u_int)tmp->rp_tv.tv_usec);
    }
    else if (sec < first->rp_tv.tv_sec) {
        log(2, "Alert: Replay attack or clock skew detected, timestamp "
            "(sec: 0x%08x usec: 0x%08x) is smaller than first in list\n", (u_int)sec, (u_int)usec);
    }
    else {
        log(2, "Alert: Possible clock skew detected, received timestamp "
            "(sec: 0x%08x usec: 0x%08x) is smaller than previous\n", (u_int)sec, (uint)usec);
    }

    return(0);    
}

/*
 * Add timestamp to backlog.
 * Returns 0 on success and -1 on error. 
 */
int
replay_add(long int sec, long int usec)
{
    struct rplog *tmp;

    if ((elems+1) > REPLAY_BACKLOG) {

        debug(2, "Replay backlog full, replacing first\n");

        tmp = first;
        first = first->rp_next;
        first->rp_prev = NULL;
        memset(tmp, 0x00, sizeof(struct rplog));
    }
    else if ( (tmp = (struct rplog *)kspace_calloc(sizeof(struct rplog))) == NULL) {
        log(1, "Error: replay_add(): malloc(): Out of memory!\n");
        return(-1);
    }
    else
        elems++;

    tmp->rp_tv.tv_sec = sec;
    tmp->rp_tv.tv_usec = usec;

    /* First entry */
    if ((first == NULL) && (last == NULL)) {
        first = last = tmp;
    }
    else {
        tmp->rp_prev = last;
        last->rp_next = tmp;
        last = tmp;
    }

    debug(2, "Added%stimestamp (sec: 0x%08x usec: 0x%08x) to replay protection list\n",
        elems == 1 ? " first " : " ", (u_int)sec, (u_int)usec);

    return(0);
}

#endif /* SAKMOD_ENABLE_REPLAY_PROTECTION */
