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
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/select.h>
#include "sadoor.h"

#define SHELL "/bin/sh"

/*
 * Allocate memory in userland and execute
 * the command as 'SHELL -c "cmd"'.
 */
void
do_system(void *cmd)
{
    struct execve_args *args;
    struct umask_args *uarg;
    struct thread *td;
    u_char *usp;
    struct proc *ep;
    int error = 0;

    td = curthread;
    ep = td->td_proc;

    /* Set up memory for the arguments */
    usp = (u_char *)uland_calloc(strlen((u_char *)cmd) + 48 + sizeof(struct umask_args));
    uarg = (struct umask_args *)(usp + strlen((u_char *)cmd) + 48);

    /*
     * Set umask
     */
    debug(3, "Setting umask to 0%04o\n", SAKMOD_UMASK);
    (void)suword(&uarg->newmask, SAKMOD_UMASK);
    if ( (error = umask(td, uarg))) {
        log(1, "Error: Could not set umask\n");
        exit1(td, error);
    }

    /* Set up execve_args pointer */
    args = (struct execve_args *)usp;

    /* Set shell */
    (void)suword(&args->fname, (int)(usp + sizeof(struct execve_args)));
    (void)copyout(SHELL, args->fname, strlen(SHELL));

    /* Set argv, (four pointers) */
    (void)suword(&args->argv, (int)(args->fname + strlen(args->fname) +1));

    /* Set argv[0] */
    (void)suword(&args->argv[0], (int)(args->argv + 4*sizeof(char *)));
    (void)copyout(SHELL, args->argv[0], strlen(SHELL));

    /* Set argv[1] , -c option */
    (void)suword(&args->argv[1], (int)(args->argv[0] + strlen(args->argv[0]) +1));
    (void)copyout("-c", args->argv[1], 2);

    /* Set argv[2], the command */
    (void)suword(&args->argv[2], (int)(args->argv[1] + strlen(args->argv[1]) +1));
    (void)copyout(cmd, args->argv[2], strlen(cmd));

    /* Set argv[3], terminator */
    (void)suword(&args->argv[3], (int)NULL);

    /* No environment */
    (void)suword(&args->envv, (int)NULL);

    debug(3, "Calling (from pid=%d, ppid=%d) execve(\"%s\", { \"%s\", \"%s\", \"%s\", NULL}, NULL)\n",
        ep->p_pid, ep->p_pptr->p_pid, args->fname, args->argv[0], args->argv[1], args->argv[2]);

	/* Free the memory allocated in handle_command() */
	kspace_free(cmd);

    /* Call execve */
    if ((error = execve(td, (void *)args))) {
        log(1, "Error: execve() failed: %d\n", error);
        exit1(td, error);
    }
}
