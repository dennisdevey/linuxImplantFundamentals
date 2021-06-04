/*
 *  File: connloop.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor connection routines.
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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <netdb.h>  /* Solaris needs this for MAXHOSTNAMELEN */
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include "random.h"
#include "sapty.h"
#include "log.h"
#include "sadoor.h"
#include "command.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

#define MY_MAX(a,b)		(((a) > (b)) ? (a) : (b))
#define BUFSIZE			8192

#ifndef SADOOR_DISABLE_ENCRYPTION 
#define DECRYPT(buffer, length) bfish_ofb8_decrypt(buffer, length)
#else
#define DECRYPT(buffer, length) {}
#endif /* SADOOR_DISABLE_ENCRYPTION */

#ifndef SADOOR_DISABLE_ENCRYPTION    
#define ENCRYPT(buffer, length) bfish_ofb8_encrypt(buffer, length)
#else
#define ENCRYPT(buffer, length) {}
#endif /* SADOOR_DISABLE_ENCRYPTION */

/* Global variables */
extern struct sa_options opt;

/* Local variables */
static int ptym;					/* Master pty */
static struct sockaddr_in client;	/* Client side */
static u_char *client_name;	

/* Local functions */
static int setwinsize(struct wininfo *);
static int connloop(int, int);
static void handle_ftrans(int, u_char, u_char *, size_t);
static void send_ftcmd(int, u_char, u_int, u_char *);


void
sigchld_handler(int signo)
{
	pid_t pid;
	int status;

	if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

		if ((opt.sao_resolve == 1) && (client_name != NULL)) {
			log_priv("Connection to %s:%u (%s) closed.\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_name);
			log_sys("Connection to %s:%u (%s) closed.",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_name);
		}
		else {
			log_priv("Connection to %s:%u closed.\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			log_sys("Connection to %s:%u closed.\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		}
#ifndef SADOOR_DISABLE_ENCRYPTION
		bfish_ofb8_cleariv();
#endif /* SADOOR_DISABLE_ENCRYPTION */
		exit(status);
	}

	signal(SIGCHLD, sigchld_handler);
}	

/*
 * Set the received window size, 
 * returns -1 on error and zero on success.
 */
static int
setwinsize(struct wininfo *winfo)
{
	struct winsize wins;

	memset(&wins, 0x00, sizeof(wins));
	wins.ws_row = ntohs(winfo->height);
	wins.ws_col = ntohs(winfo->width);
	wins.ws_xpixel = ntohs(winfo->xpix);
	wins.ws_ypixel = ntohs(winfo->ypix);

	if (opt.sao_privbose > 2)
		log_priv("Received Window size (rows: %u columns: %u)\n", 
			wins.ws_row, wins.ws_col);

	if (ioctl(ptym, TIOCSWINSZ, &wins) < 0) {
		log_priv("Error setting window size: %s\n",
			strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * Create session key, fix environment, 
 * set up pty and run shell.
 */
int
handle_connection(int sock_fd, struct bfish_key *bk)
{
    int master_fd;
	pid_t mpid; /* master PID */
	int addrlen = sizeof(struct sockaddr_in);
	u_char client_addr[24];
	u_char slave_name[20];

	if (getpeername(sock_fd, (struct sockaddr *)&client, &addrlen) < 0) {
		log_sys("Error receiving address of connecting client, exiting\n");
		exit(EXIT_FAILURE);
	}

	snprintf(client_addr, sizeof(client_addr), "%s:%u",
		inet_ntoa(client.sin_addr), ntohs(client.sin_port));

	
	/* Syslog only needs to know that a connection is 
	 * established, not how it was created */
	if (opt.sao_sysvbose > 0) {

		if ((opt.sao_resolve == 1) && ((client_name = net_hostname(&client.sin_addr)) != NULL))
			log_sys("Connection established with %s (%s)", client_addr, client_name);
		else
			log_sys("Connection established with %s", client_addr);
	}

	/* Generate session key and IV's */ 
#ifndef SADOOR_DISABLE_ENCRYPTION
	{
		struct sesskey tmpkey;
		struct sesskey sekey;
				
		if (random_bytes((u_char *)&sekey, sizeof(sekey)) == NULL) {
			log_priv("Error generating random bytes for session key (%s), bailing out\n", strerror(errno));
			log_sys("Error generating random bytes for session key (%s), bailing out", strerror(errno));
			exit(EXIT_FAILURE);
		}

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
				sizeof(sekey)-sizeof(sekey.iv), sekey.iv, 8, bk);
		
		if (opt.sao_privbose > 2)
			log_priv("Sending session key\n");

		/* Send session key to client */
		if (writen(sock_fd, (u_char *)&tmpkey, sizeof(tmpkey)) != sizeof(tmpkey)) {
			log_sys("Error: writen(): %s", strerror(errno));
			log_priv("Error: writen(): %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Set up the new key to use */
		if (bfish_ofb8_setiv(sekey.key, sizeof(sekey.key), 
				sekey.deciv, sekey.enciv) == -1)
			exit(EXIT_FAILURE);

		/* Zero out old */
		memset(&tmpkey, 0x00, sizeof(tmpkey));
		memset(&sekey, 0x00, sizeof(sekey));
		memset(bk, 0x00, sizeof(struct bfish_key));
		free(bk);
	}
#endif /* SADOOR_DISABLE_ENCRYPTION */

	if ( (mpid = pty_fork(&master_fd, slave_name, NULL, NULL)) < 0)
        exit(EXIT_FAILURE);

    /* Child, run program */
    if (mpid == 0) {
		char ecli[sizeof(SADOOR_ENV_CLIENTADDR)+48];
		char eclin[sizeof(SADOOR_ENV_CLIENTNAME)+MAXHOSTNAMELEN+1];
		char *ev[] = { ecli, eclin, NULL};
	
		snprintf(ecli, sizeof(ecli), "%s=%s", SADOOR_ENV_CLIENTADDR, client_addr);
		
		if ((opt.sao_resolve == 1) && (client_name != NULL))
			snprintf(eclin, sizeof(eclin), "%s=%s", SADOOR_ENV_CLIENTNAME, client_name);
		
		execle(opt.sao_shell, basename(opt.sao_shell), (char *)NULL, ev);
		log_sys("Error: execlp(): %s\n", strerror(errno));
		log_priv("Error: execlp(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
	
	ptym = master_fd;
	
	/* Parent continues */
	return(connloop(sock_fd, master_fd));
}

/*
 * Main connection loop, reads and writes 
 * data between pty and socket.
 * Returns 0 on succes and 1 on error;
 */
int
connloop(int sock_fd, int master_fd)
{
	u_char ptybuf[BUFSIZE];
	u_char sockbuf[BUFSIZE];
	ssize_t topty;
	ssize_t tosock;
	fd_set readset;
	ssize_t i;
	int nfd;

	signal(SIGTTOU, SIG_IGN);	
	signal(SIGCHLD, sigchld_handler);

    if ( (nfd = MY_MAX(sock_fd, master_fd) +1) > FD_SETSIZE) {
        log_priv("Error: FD_SETSIZE to small!\n");
        kill(0, SIGTERM);
        return(1);
    }
	FD_ZERO(&readset);
	FD_SET(sock_fd, &readset);
	FD_SET(master_fd, &readset);

	for (;;) {
		fd_set readtmp;
		memcpy(&readtmp, &readset, sizeof(readtmp));
		
		/* Read data */
		if (select(nfd, &readtmp, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) 
				continue;
			
			log_priv("Error: select(): %s\n", strerror(errno));
			kill(0, SIGTERM);
			return(1);
		}

		if (FD_ISSET(sock_fd, &readtmp)) {
			u_char *bufpt = ptybuf;
			
			if ( (topty = read(sock_fd, ptybuf, sizeof(ptybuf))) < 0) {
				log_priv("Error: read from socket: %s\n", strerror(errno));
				break;
			}

			if (topty == 0)
				break;
			
			DECRYPT(ptybuf, topty);

            /* Scan for magic bytes */
			if ( (i = memstr(ptybuf, topty, MAGIC_BYTES, MAGIC_BYTES_LEN)) >= 0) {
				
				/* Write leading data to pty */
				if (writen(master_fd, ptybuf, i) != i) {
					log_priv("Error: write to pty: %s\n", strerror(errno));
					kill(0, SIGTERM);
					continue;
				}
				bufpt = &ptybuf[i];
				topty -= i;

				if ( (i += MAGIC_BYTES_LEN) > topty) {
					log_priv("Error: data to short to contain any command\n");
					continue;
				}

				/* Command code */
				switch (ptybuf[i++]) {
				
					/* New window size, set the new window size and write
					 * the rest of data to pty. */
					case CONN_WINCHANGED:
						{
							struct wininfo winfo;

							if ((i + sizeof(struct wininfo)) > topty) {
									log_priv("Error: Received data is to short to "
									"contain window size\n");
								topty -= i;
								break;
							}
							
							/* Set the new window size, we need to copy
							 * to avoid SIGBUS ... */
							memcpy(&winfo, &ptybuf[i], sizeof(winfo));
							setwinsize(&winfo);

							i += sizeof(struct wininfo);
							bufpt = &ptybuf[i];
							topty -= i;
							
						}
						break;
				
					/* File transfer data */
					case CONN_FTDATA:
						{
							struct ftcmd ftc;

							if ((i + sizeof(struct ftcmd)) > topty) {
								log_priv("Error: Received data is to short to "
									"contain file transfer header\n");
								topty -= i;
								break;
							}

							memcpy(&ftc, &ptybuf[i], sizeof(ftc));
							ftc.length = ntohl(ftc.length);
							i += sizeof(struct ftcmd);

							if (ftc.length > topty) {
								log_priv("Error: Received data is to short to "
									"contain file transfer data\n");
								topty -= i;
								break;
							}
							
							i += ftc.length;
							topty -= i;
							bufpt = &ptybuf[(i > sizeof(ptybuf)) ? 0 : i];
							
							/* Make sure that data not belongin to file transfer 
							 * is written to the pty in correct order. */
							if (ftc.code == FT_PUTFILE) {
								
								if ( (topty > 0) && (writen(master_fd, bufpt, topty) != topty)) {
									log_priv("Error: write to pty: %s\n", strerror(errno));
									break;
								}
								topty = 0;
							}

							handle_ftrans(sock_fd, ftc.code, &ptybuf[i-ftc.length], ftc.length);
						}
						break;

					/* Bad command code (?), write everything, 
					 * including the magic bytes to pty */
					default:
						if (opt.sao_privbose > 1) {
							log_priv("Error: Unrecognized command code (0x%02x)\n",
								ptybuf[i-1]);
						}
						break;
				}
			}

			/* Write (remaining ?) data to pty */
			if ( (topty > 0) && (writen(master_fd, bufpt, topty) != topty)) {
                log_priv("Error: write to pty: %s\n", strerror(errno));
                break;
            }
		}

		if (FD_ISSET(master_fd, &readtmp)) {
			if ( (tosock = read(master_fd, sockbuf, sizeof(sockbuf))) < 0) {
				log_priv("Error: read from pty: %s\n", strerror(errno));
				break;
			}
			if (tosock == 0)
				break;
			
			ENCRYPT(sockbuf, tosock);

			if (writen(sock_fd, sockbuf, tosock) != tosock) {
				log_priv("Error: write to socket: %s\n", strerror(errno));
				break;
			}
		}
	}

	kill(0, SIGTERM);
	return(1);
}

/*
 * Respond to received file transfer request, either with an error 
 * message, or by sending FT_STORE or FT_READ command back.
 */
static void
handle_ftrans(int sock_fd, u_char code, u_char *data, size_t dlen)
{
	u_char buf[BUFSIZE];	
	struct stat sb;			
	struct ftcmd ftc;		
	u_char *path;			/* Path to file */	
	ssize_t i;
	ssize_t len;
	ssize_t bsent;
	int fd = -1;

	path = data;

	/* File transfer disabled */
	if (opt.sao_noftrans == 1) {
		
		send_ftcmd(sock_fd, FT_ERROR, 
			sizeof(FTRANS_DISABLED_ERRORMSG), FTRANS_DISABLED_ERRORMSG);
		return;
	}

	/* Find end of first string */
	for (i = 0; i < dlen; i++) {
		if (data[i] == '\0') 
			break;
	}	

	if (i >= dlen) {
		log_priv("Error: Malformated file-path "
			"string in file transfer request|n");
		return;
	}

	switch(code) {
	
		/* Send file */
		case FT_GETFILE:
			log_priv("Received GET FILE %s\n", path);

            /* Check access and get size on local file, If it fails, 
			 * send error message and return. */
            if (( (fd = open(path, O_RDONLY)) == -1) || 
					(fstat(fd, &sb) == -1)) {

				if (opt.sao_privbose)
					log_priv("Error reading local file for transfer: %s\n", 
						strerror(errno));
				
				/* Send error message */
				send_ftcmd(sock_fd, FT_ERROR, strlen(strerror(errno))+1, 
					strerror(errno));
                return;
            }
			
			/* Send header */
			send_ftcmd(sock_fd, FT_STORE, sb.st_size, NULL);

			if (opt.sao_privbose > 1) 
				log_priv("Sending file '%s' (%u bytes)\n", path, 
					sb.st_size);
			
			bsent = 0;
			while (bsent < sb.st_size) {
				len = (sb.st_size - bsent) > sizeof(buf) ? sizeof(buf) : (sb.st_size - bsent);				
		
				/* Read a block */
				if (readn(fd, buf, len) != len) {
					log_priv("Error: readn(): %s\n", strerror(errno));
					log_sys("Error: readn(): %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				
				ENCRYPT(buf, len);
			
				/* Write the file */
				if (writen(sock_fd, buf, len) != len) {
					log_priv("Error: writen(): %s\n", strerror(errno));
					log_sys("Error: writen(): %s", strerror(errno));
					exit(EXIT_FAILURE);
				}
				bsent += len;
			}

			if (opt.sao_privbose > 1) 
				log_priv("File '%s' (%u bytes), successfully sent\n", path, 
					sb.st_size);
			break;

		/* Store file */
		case FT_PUTFILE:
			log_priv("Received PUT FILE %s\n", path);

			/* Check if file can be stored */
            if ( (fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) == -1) {

                if (opt.sao_privbose)
                    log_priv("Error opening local file for writing: %s\n",
                        strerror(errno));

                /* Send error message */
                send_ftcmd(sock_fd, FT_ERROR, strlen(strerror(errno))+1,
                    strerror(errno));
                return;
            }

			/* Send read command and wait for FT_READ_ACK */
			send_ftcmd(sock_fd, FT_READ, 0, NULL);
			
			/* Read until we find magic bytes */
			len = 0;
			i = -1;
			do {
				if (read(sock_fd, &buf[len], 1) != 1) {
					log_priv("Error: read(): %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				
				DECRYPT(&buf[len], 1);
				
			} while ( (len<(sizeof(buf)-2)) && 
				(i = memstr(buf, ++len, MAGIC_BYTES, MAGIC_BYTES_LEN)) == -1);	

			if (read(sock_fd, &buf[len], 1) != 1) {
				log_priv("Error: read(): %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			
			DECRYPT(&buf[len], 1);

			if (i == -1) {
				log_priv("Error: Did not receive awaiting FT_READ_ACK, exiting\n");
				exit(EXIT_FAILURE);
			}

			/* Attempt to write prepended data to pty */
			if ((i > 0) && ready_write(ptym)) {
				if ( (i > 0) && writen(ptym, buf, i) != i) {
					log_priv("Error writen(): %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
			else if (i > 0) 
				log_priv("Warning: Master PTY not ready for writing, "
					"%u bytes will not be transfered\n", (u_int)i);

			if (readn(sock_fd, &ftc, sizeof(ftc)) != sizeof(ftc)) {
				log_priv("Error: readn(): %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			
			DECRYPT((uint8_t *)&ftc, sizeof(ftc));
			
			ftc.length = ntohl(ftc.length);

			if (ftc.code != FT_READ_ACK) {
				log_priv("Received unvalid file transfer code while "
						"waiting for FT_READ_ACK\n");
				exit(EXIT_FAILURE);
			}
		
			/* Read file */
			{
				size_t rn;

				while (ftc.length > 0) {
					rn = (ftc.length > sizeof(buf)) ? sizeof(buf) : ftc.length;

					if ( (i = readn(sock_fd, buf, rn)) != rn) {
						log_priv("Error: readn(): %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
				
					DECRYPT(buf, rn);
					
					if (writen(fd, buf, rn) != rn) {
						log_priv("Error: writen(): %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					ftc.length -= rn;
				}
			}
			log_priv("File '%s' sucessfully stored\n", path);
			break;

		/* Can't happen here */
		case FT_STORE:
		case FT_ERROR:
		default:
			log_priv("Error: Unrecognized file "
					"transfer code (0x%02x)\n", code);
			break;
	}

	/* Close file */
	if ( (fd > 0) && (close(fd) < 0))
		log_priv("Error: close(): %s\n", strerror(errno));

	return;
}

/*
 * Send file transfer command header.
 * dlen should be host endian.
 * Data is not copyed if it's a NULL pointer.
 */
static void
send_ftcmd(int sock_fd, u_char code, u_int dlen, u_char *data)
{
    u_char outbuf[1024];
    size_t i;
    struct ftcmd ftc;

    if ((data != NULL) && 
			((dlen + MAGIC_BYTES_LEN + sizeof(ftc)) > sizeof(outbuf))) {
        log_sys("Error: File tranfer command  is larger than buffer");
        log_priv("Error: File tranfer command  is larger than buffer\n");
        exit(EXIT_FAILURE);
    }

    /* Set magic bytes */
    i = MAGIC_BYTES_LEN;
    memcpy(outbuf, MAGIC_BYTES, MAGIC_BYTES_LEN);

    /* Set command code */
    outbuf[i++] = CONN_FTDATA;

    /* Add FT header */
    ftc.code = code;
    ftc.length = htonl(dlen);
    memcpy(&outbuf[i], &ftc, sizeof(struct ftcmd));
    i += sizeof(struct ftcmd);

    /* Add data if not NULL */
    if (data != NULL) {
        memcpy(&outbuf[i], data, ntohl(ftc.length));
        i += ntohl(ftc.length);
    }

	ENCRYPT(outbuf, i);

    /* Write message */
    if (writen(sock_fd, outbuf, i) != i) {
        log_priv("Error: writen(): %s\n", strerror(errno));
        log_sys("Error: writen(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

