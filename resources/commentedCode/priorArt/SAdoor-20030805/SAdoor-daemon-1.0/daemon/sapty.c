/*
 *  File: sapty.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor PTY routines.
 *  Version: 1.0
 *  Date: Tue Jan  7 23:24:15 CET 2003
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


/*
 * Most of the contents in this file is taken from 
 * Advanced Progr. In the UNIX env. By W. Richard Stevens.
 */
 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <grp.h>
#ifdef  SYSVREL4PTY
#include <stropts.h>
#endif
#include "log.h"

/* SYSV RELEASE 4 PTY routines */
#ifdef SYSVREL4PTY

/*
 * Open master pty.
 * The name is stored in pts_name, which must be at least
 * 20 bytes long.
 * Returns file descriptor of master pty on success,
 * negative value on error.
 */
int
ptymaster_open (char *pts_name)
{
    char *ptr;
    int fdm;

    strcpy (pts_name, "/dev/ptmx");
    if ( (fdm = open (pts_name, O_RDWR)) < 0)
        return(-1);

	/* Grant access to slave */
    if (grantpt (fdm) < 0) {
        close(fdm);
        return(-2);
    }

	/* Clear slave's lock flag */
    if (unlockpt (fdm) < 0) {
        close(fdm);
        return(-3);
    }

	/* Get name of slave */
    if ( (ptr = ptsname (fdm)) == NULL) {
        close(fdm);
        return(-4);
    }
	
	/* Return name of slave */
    strcpy(pts_name, ptr);

	/* Return master file descriptor */
    return(fdm);
}

/*
 * Open slave pty with filename pointed to by pts_name.
 * The opened file is bound to file descriptor of master pty (fdm).
 * Returns the file descriptor of slave pty on succes, 
 * Negativ value on error.
 */
int
ptyslave_open (int fdm, char *pts_name)
{
    int fds;

    if ( (fds = open (pts_name, O_RDWR)) < 0) {
        close(fdm);
        return(-5);
    }

    if (ioctl(fds, I_PUSH, "ptem") < 0) {
        close(fdm);
        close(fds);
        return(-6);
    }

    if (ioctl(fds, I_PUSH, "ldterm") < 0) {
        close(fdm);
        close(fds);
        return(-7);
    }

    if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
        close(fdm);
        close(fds);
        return(-8);
    }

    return (fds);
}


/* 4.3+BSD Routines */
#else 

/*
 * Open pty master (4.3+BSD).
 * Scan /dev/ for the next available pty device.
 * The argument must be big enough to hold the
 * name of the actual device (12 bytes).
 * Returns -1 on error and the file descriptor of
 * the master on success.
 */
int
ptymaster_open(u_char *pts_name)
{
    int fd;
    u_char *pt1, *pt2;

    strcpy(pts_name, "/dev/ptyXX");

    for (pt1 = "pqrstuvwxyzPQRST"; *pt1 != '\0'; pt1++) {
        pts_name[8] = *pt1;

        for (pt2 = "0123456890abcdef"; *pt2 != '\0'; pt2++) {
            pts_name[9] = *pt2;


            if ( (fd = open(pts_name, O_RDWR)) < 0) {

                if (errno == ENOENT) /* No more pty devices */
                    return(-1);
                else
                    continue;
            }

            pts_name[5] = 't'; /* Change "pty" to "tty" */
            return(fd);
        }
    }

    return(-1);
}

/*
 * Open slave pty with filename pointed to by pts_name.
 * The opened file is bound to file descriptor of master pty (fdm).
 * Returns the file descriptor of slave pty on succes, -1 on error.
 */
int
ptyslave_open(int master_fd, u_char *pts_name)
{
    struct group *gpt;
    int gid;
    int fd;

    /* Check for "tty" in group file */
    if ( (gpt = getgrnam("tty")) != NULL)
        gid = gpt->gr_gid;
    else
        gid = -1;

    /* Only works for root */
    chown(pts_name, getuid(), gid);
    chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP);

    if ( (fd = open(pts_name, O_RDWR)) < 0) {
        close(master_fd);
        return(-1);
    }
    return(fd);
}

#endif /* SYSVREL4PTY */

/*
 * Fork and bind child (slave pty) to parent (master pty).
 * Returns the same as fork(2).
 */
pid_t
pty_fork(int *ptrfdm, u_char *slave_name,
        const struct termios *slave_termios,
        const struct winsize *slave_winsize)
{
    int fd_master;
    int fd_slave;
    pid_t pid;
    u_char pts_name[20];

    /* Open pty master */
    if ( (fd_master = ptymaster_open(pts_name)) < 0) {
        log_sys("Error opening master pty: %s", pts_name);
        return(-1);
    }

    /* Copy slave name */
    if (slave_name != NULL)
        strcpy(slave_name, pts_name);

    if ( (pid = fork()) < 0) {
        log_sys("Error: fork(): %s", strerror(errno));
        return(-1);
    }


    /* Child */
    else if (pid == 0) {

        /* Create a new session (seesion leader,
         * group leader and no controlling terminal)*/
        if (setsid() < 0) {
			log_sys("Error: setsid(): %s", strerror(errno));
            return(-1);
        }

        /* Open slave pty */
        if ( (fd_slave = ptyslave_open(fd_master, pts_name)) < 0) {
            log_sys("Error opening slave pty: %s: %s", 
				pts_name, strerror(errno));
            return(-1);
        }

        /* No more need for master in child */
        if (close(fd_master) < 0)
			log_sys("Error: close(): %s", strerror(errno));

/* 4.3+BSD way to acquire controlling terminal, we use !CIBAUD
 * to avoid doing this under SunOS */
#if defined(TIOCSCTTY) && !defined(CIBAUD)

        if (ioctl(fd_slave, TIOCSCTTY, (u_char *)NULL) < 0) {
            log_sys("Error: ioctl(TIOCSCTTY)", strerror(errno));
            return(-1);
        }
#endif

        /* Set termios for slave */
        if (slave_termios != NULL) {
            if (tcsetattr(fd_slave, TCSANOW, slave_termios) < 0) {
                log_sys("Error: tcsetattr on slave pty\n");
                return(-1);
            }
        }

        /* Set window size for slave */
        if (slave_winsize != NULL) {
            if (ioctl(fd_slave, TIOCSWINSZ, slave_winsize) < 0) {
                log_sys("Error: TIOCSWINSZ on slave pty");
                return(-1);
            }
        }

        /* Slave becomes stdin/stdout/stderr of master */
        if (dup2(fd_slave, STDIN_FILENO) != STDIN_FILENO) {
            log_sys("Error: dup2(.., STDERR_FILENO): %s", strerror(errno));
            return(-1);
        }
		
        if (dup2(fd_slave, STDOUT_FILENO) != STDOUT_FILENO) {
            log_sys("Error: dup2(.., STDOUT_FILENO): %s", strerror(errno));
            return(-1);
        }

        if (dup2(fd_slave, STDERR_FILENO) != STDERR_FILENO) {
			log_sys("Error: dup2(.., STDERR_FILENO): %s", strerror(errno));
            return(-1);
        }

        if (fd_slave > STDERR_FILENO) {
            if (close(fd_slave) < 0)
				log_sys("Error: close(): %s", strerror(errno));
		}

        /* Return zero just like fork() (child) */
        return(0);
    }
    else {
        *ptrfdm = fd_master;
        return(pid);    /* Child pid */
    }
}
