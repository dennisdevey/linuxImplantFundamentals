/*
 *  File: conn.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: TCP/IP accept/listen routines.
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"
#include "sadc.h"
#include "sadoor.h"
#include "net.h"

/* Global options */
extern struct sa_options opt;

void
timeout_handler(int signo)
{
	log_priv("Connection timed out after %u seconds.\n", 
		(u_int)opt.sao_ctout);
	exit(EXIT_FAILURE);
}

/*
 * Create socket and bind it to the selected port.
 * If port is zero, the next available port is selected
 * by the system.
 * Returns the socket file descriptor on success, -1 on error.
 * lip - The local ip to accept connections on.
 * port - The port to bind to.
 * lip and port is assumed to be in network byte order.
 * If addloc is not NULL, the address will be copyed to that location.
 */
int
conn_bindsock(long lip, u_short port, struct sockaddr_in *addloc)
{
	struct sockaddr_in laddr;
	int lsock;

	/* Create IPv4 socket */
	if ( (lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_priv("Error: socket(): %s\n", strerror(errno));
		return(-1);
	}
	
	/* Create IPv4 address */
	memset(&laddr, 0x00, sizeof(struct sockaddr_in));
	laddr.sin_family = AF_INET;
	laddr.sin_port = port;       
	laddr.sin_addr.s_addr = lip; 
	
	/* Bind socket to address */
	if ( bind(lsock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		log_priv("Error: bind(): %s\n", strerror(errno));
		return(-1);
	}	
	
	/* Copy address */
	if (addloc != NULL) {
		int len = sizeof(struct sockaddr_in);
		getsockname(lsock, (struct sockaddr *)addloc, &len); 
	}

	return(lsock);
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
conn_accept(int lsock, time_t timeout, u_long cip, u_short cport)
{
    struct sockaddr_in client;
    u_int addrlen = sizeof(struct sockaddr_in);
    int sa_fd;
	u_char *pt;


    /* Listen for connections on the bound address
     * (with a queue of 0 waiting clients for the current socket) */
     if (listen(lsock, 1) < 0) {
		log_priv("Error: listen(): %s\n", strerror(errno));
        return(-1);
     }

	/* Set timeout */
	signal(SIGALRM, timeout_handler);
	alarm(timeout);

    /* Accept connection on the listening socket */
    if ( (sa_fd = accept(lsock, (struct sockaddr *)&client,
             &addrlen)) < 0) {
        log_priv("Error: accept(): %s\n", strerror(errno));
        return(-1);
    }
	
	/* Disable alarm */
	alarm(0);

	/* Close local socket */
	if (close(lsock) < 0)
		log_sys("Warning, failed to close socket: close(): %s", strerror(errno));

    /* Get address of "client" that connected */
    if (getpeername(sa_fd, (struct sockaddr *)&client, &addrlen) < 0) {
        log_priv("Error receiving address of connecting client, exiting\n");
        return(-1);
    }

	/* Check that address match */
	if ( ((cip != 0) && (client.sin_addr.s_addr != cip)) || 
			(((cport != 0) && client.sin_port != cport))) {
		log_priv("Error: address of connecting client (%s:%u) do not match "
			"expected address (%s:%u)\n", 
			inet_ntoa(client.sin_addr), ntohs(client.sin_port),
			net_ntoa(cip, NULL), ntohs(cport));
		return(-1);
	}

		
	if ((opt.sao_resolve == 1) && ((pt = net_hostname(&client.sin_addr)) != NULL)) {
   		log_priv("%s:%u (%s) connected\n", inet_ntoa(client.sin_addr), 
			ntohs(client.sin_port), pt);
	}
	else
		log_priv("%s:%u connected\n", inet_ntoa(client.sin_addr), 
			ntohs(client.sin_port));

	return(sa_fd);
}

/*
 * Connect to ip:port.
 * Ip and port is assumed to be in network byte order.
 * Returns socket descriptor on success, -1 on error.
 */
int
conn_connect(u_long ip, u_short port)
{
    struct sockaddr_in taddr;
    int sock_fd;

    /* Create IPv4 address */
    memset(&taddr, 0x00, sizeof(struct sockaddr_in));
    taddr.sin_addr.s_addr = ip;
    taddr.sin_port = port;
    taddr.sin_family = AF_INET;

    /* Create socket */
    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_priv("Error: socket(): %s\n", strerror(errno));
        return(-1);
    }

    /* Pick the next available source port and connect to client */
    if (connect(sock_fd, (struct sockaddr *)&taddr, sizeof(taddr)) < 0) {
        log_priv("Error: connect to %s:%u: %s\n", net_ntoa(ip, NULL), 
			ntohs(port), strerror(errno));
        return(-1);
    }

    if (opt.sao_privbose > 0) {
		u_char *pt;
		
        if ( (opt.sao_resolve == 1) && 
				((pt = net_hostname(&taddr.sin_addr)) != NULL)) {

			log_priv("Connected to %s:%u (%s)\n", inet_ntoa(taddr.sin_addr),
				ntohs(taddr.sin_port), pt);
        }
		else
			log_priv("Connected to %s:%u\n", inet_ntoa(taddr.sin_addr),
				ntohs(taddr.sin_port));
    }

    return(sock_fd);
}

