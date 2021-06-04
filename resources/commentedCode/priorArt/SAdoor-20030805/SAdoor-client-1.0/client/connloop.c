/*
 *  File: connloop.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Connection handling routines
 *  Version: 1.0
 *  Date: Mon Mar 17 20:11:03 CET 2003
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <libgen.h>
#include "sash.h"
#include "bfish.h"
#include "command.h"

#define BUFSIZE 8192

#define PROGBAR_INIT		0x0
#define PROGBAR_UPDATE		0x1
#define RAISE_SIGWINCH		0x1
#define DONT_SEND_WINZ		0x2

#ifndef SADOOR_DISABLE_ENCRYPTION
#define DECRYPT(buffer, length)  bfish_ofb8_decrypt(buffer, length)
#else
#define DECRYPT(buffer, length) {}
#endif /* SADOOR_DISABLE_ENCRYPTION */

#ifndef SADOOR_DISABLE_ENCRYPTION
#define ENCRYPT(buffer, length)  bfish_ofb8_encrypt(buffer, length)
#else
#define ENCRYPT(buffer, length) {}
#endif /* SADOOR_DISABLE_ENCRYPTION */

/* Global Options */
extern struct sashopt opt;

/* Private functions */
static void restore_tty(void);
static void handle_ftcmd(int, u_char, int);
static void print_pbar(u_char);

/* Private variables */
static struct termios tio;
static u_char winflags = 0;	/* RAISE_SIGWINCH, DONT_SEND_WINZ */	
static int socket_fd;
static size_t b2send;		/* Bytes to send, used by print_pbar() */
static size_t bsent;        /* Bytes sent, used by print_pbar() */
static u_char *tfile;		/* Name of file to transfer, used by print_pbar() */

/*
 * Write window size to socket if it is ready for writing, 
 * otherwise flag that window size has changed.
 */
void
sigwinch_handler(int signo)
{
	u_char buf[32];
	struct winsize wins;
	struct wininfo winfo;
	size_t n;

	winflags |= RAISE_SIGWINCH;
		
	/* We are currently in file transfer mode */
	if (winflags & DONT_SEND_WINZ) {
		signal(SIGWINCH, sigwinch_handler);
		return;
	}

	/* Socket will not block */
	if (ready_write(socket_fd) == 1) {

		if (ioctl(STDIN_FILENO, TIOCGWINSZ, &wins) < 0) {
			fprintf(stderr, "** Error retreiving current window size: %s\n",
				strerror(errno));
		}

		/* Set magic bytes */
		n = MAGIC_BYTES_LEN;
		memcpy(buf, MAGIC_BYTES, MAGIC_BYTES_LEN);

		/* Set command code */
		buf[n++] = CONN_WINCHANGED;

		/* Append window information */
		winfo.height = htons(wins.ws_row);
		winfo.width = htons(wins.ws_col);
		winfo.xpix = htons(wins.ws_xpixel);
		winfo.ypix = htons(wins.ws_ypixel);
		memcpy(&buf[n], &winfo, sizeof(winfo));
		n += sizeof(winfo);

		ENCRYPT(buf, n);
		
		/* Write information to socket */
		if (writen(socket_fd, buf, n) != n)
			fprintf(stderr, "** Error writing window size to socket: %s\n",
				strerror(errno));
		winflags &= ~RAISE_SIGWINCH;
	}
	signal(SIGWINCH, sigwinch_handler);
}

/*
 * atexit(3) function.
 * Restores saved TTY settings.
 */
static void
restore_tty(void)
{
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &tio) < 0)
		fprintf(stderr, "** Error retoring TTY settings "
			"(try reset(1) or tset(1)): %s\n", strerror(errno));
	printf("\nConnection closed by foreign host.\n");
}


/*
 * Connection loop
 */
int
connloop(int sock_fd, struct bfish_key *bk)
{
	
	u_char sockbuf[48];			/* Stdin buffer */
	u_char outbuf[BUFSIZE];		/* Stdout buffer */
	int fdsave;					/* File opened by escape_loop() */
	struct ftcmd ftc;			/* File transfer command */
	ssize_t toout;				/* Number of bytes to write to stdout */
	ssize_t tosock;				/* Number of bytes to write to socket */
	fd_set readset;				/* STDIN_FILENO, sock_fd */
	u_char *bufpt;				/* Points into outbuf */
	u_char *lfile = NULL;		/* Local file to transfer in some direction */
	int nfd;

#ifndef SADOOR_DISABLE_ENCRYPTION
	struct sesskey sekey;		/* Session key */

	/* Read session key */
	if (readn(sock_fd, &sekey, sizeof(sekey)) != sizeof(sekey)) {
		fprintf(stderr, "** SASH Error reading session key: %s\n", 
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* All IV's are already in network byte order */
	bfish_cfb_decrypt(&sekey.code, sizeof(sekey)-sizeof(sekey.iv), sekey.iv, 8, bk);

	/* Just check that we decrypted it correct */
	if (sekey.code != CONN_SESSION_KEY) {
		fprintf(stderr, "** SASH Error: Decryption of session key failed\n");
		exit(EXIT_FAILURE);
	}

	if (opt.sa_verbose > 1)
		printf("Received session key\n");

	/* Set up the new key to use */
	if (bfish_ofb8_setiv(sekey.key, sizeof(sekey.key), 
			sekey.enciv, sekey.deciv) == -1)
		exit(EXIT_FAILURE);

	/* Zero out old key */
	memset(&sekey, 0x00, sizeof(sekey));
	memset(bk, 0x00, sizeof(struct bfish_key));
	free(bk);

	/* Better safe than sorry ... */
	if (atexit(bfish_ofb8_cleariv) < 0) {
		fprintf(stderr, "** SASH Error: atexit(): %s\r\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

#endif /* SADOOR_DISABLE_ENCRYPTION */

    if (tty_raw(STDIN_FILENO, &tio) < 0) {
        fprintf(stderr, "tty_raw() error\n");
        exit(EXIT_FAILURE);
    }

	if (atexit(restore_tty) < 0) {
		fprintf(stderr, "** SASH Error: atexit(): %s\r\n", 
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ( (nfd = sock_fd +1) > FD_SETSIZE) {
		fprintf(stderr, "** SASH Error: FD_SETSIZE to small!\r\n");
		exit(EXIT_FAILURE);
	}
	socket_fd = sock_fd;

	FD_ZERO(&readset);
	FD_SET(sock_fd, &readset);
	FD_SET(STDIN_FILENO, &readset);

	signal(SIGWINCH, sigwinch_handler);

	/* Send window size first time through */
	winflags |= RAISE_SIGWINCH;
	fdsave = -1; /* No file transfer in progres */
	
	if (opt.sa_verbose > 1)
		fprintf(stdout, "Sending window size\r\n");

	for (;;) {
		fd_set readtmp;
		memcpy(&readtmp, &readset, sizeof(readtmp));
	
		/* Raise SIGWINCH if the new window size has not been sent */
		if (winflags & RAISE_SIGWINCH) {
			if (raise(SIGWINCH) < 0) 
				fprintf(stderr, "** SASH Error: raise(): %s\r\n", 
					strerror(errno));
		}

		/* Read data */
		if (select(nfd, &readtmp, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) 
				continue;
			
			fprintf(stderr, "** SASH Error: select(): %s\r\n", 
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Read from keyboard */
		if (FD_ISSET(STDIN_FILENO, &readtmp)) {
			
			if ( (tosock = read(STDIN_FILENO, sockbuf, 1)) < 0) {
				fprintf(stderr, "** SASH Error: read from stdin: %s\n", 
					strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (tosock == 0) 
				exit(EXIT_SUCCESS);
			
			/* Look for escape character */ 
			if (sockbuf[0] == opt.sa_esc) {
				
				/* Only one file transfer at the time */
				if (fdsave < 0) {
					if ( (lfile = escape_loop(sock_fd, &tio, &fdsave)) != NULL)
						tfile = basename(lfile);
				}
				else
					fprintf(stderr, "** SASH: Ignoring escape sequence, "
						"awaiting file command\r\n");
				tosock = 0;
			}
	
			ENCRYPT(sockbuf, tosock);
		
			if ((tosock != 0) && (writen(sock_fd, sockbuf, tosock) != tosock)) {
				fprintf(stderr, "** SASH Error: write to socket: %s\n", 
					strerror(errno));
				exit(EXIT_FAILURE);
			}
		}

		/* Read from socket */
        if (FD_ISSET(sock_fd, &readtmp)) {
            if ( (toout = read(sock_fd, outbuf, sizeof(outbuf))) < 0) {
                fprintf(stderr, "** SASH Error: read from socket: %s\r\n",
                    strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (toout == 0)
                exit(EXIT_SUCCESS);
			
			DECRYPT(outbuf, toout);
			bufpt = outbuf;	
			
			/* Scan for escape sequence if we are waiting for a file command */
			if (fdsave > 0) {
				ssize_t i;

				if ( (i = memstr(outbuf, toout, MAGIC_BYTES, MAGIC_BYTES_LEN)) >= 0) {
					
					/* Write leading data to stdout */
					if (i && writen(STDOUT_FILENO, outbuf, i) != i) {
						fprintf(stderr, "** SASH Error: write to STDOUT: %s\n", 
							strerror(errno));
						exit(EXIT_FAILURE);
					}

					bufpt = &outbuf[i];
					
					/* Read again to get code, should never happend */
					if ( (i + MAGIC_BYTES_LEN) > toout) {
						fprintf(stderr, "** SASH Error: data to short "
							"to contain any command, reading again ..\r\n");

						if ( (toout = read(sock_fd, outbuf, sizeof(outbuf))) < 0) {
							fprintf(stderr, "** SASH Error: read from socket: %s\r\n",
								strerror(errno));
							exit(EXIT_FAILURE);
						}
						i = 0;
					}
					else {
						toout -= MAGIC_BYTES_LEN +i;
						i += MAGIC_BYTES_LEN;
					}

					/* Command code */
					switch (outbuf[i++]) {
						
						/* The only one expected, we know for sure that this is an answer
						 * to a previous request */
						case CONN_FTDATA:
							toout--;
					
							if (sizeof(struct ftcmd) > toout) {
								fprintf(stderr, "** SASH Error: Received data is to short to "
									"contain file transfer header\n");
								break;
							}

							memcpy(&ftc, &outbuf[i], sizeof(ftc));
							ftc.length = ntohl(ftc.length);
							i += sizeof(struct ftcmd);
							toout -= sizeof(struct ftcmd);
							
							/* If it's an error message, write it and continue */
							if (ftc.code == FT_ERROR) {
		
								if (ftc.length > toout) {
									fprintf(stderr, "** SASH Error: Received To big error message\r\n");
									continue;
								}
								
								/* Just to make sure */
								outbuf[i+ftc.length] = '\0';
							
								fprintf(stderr, "** SASH File transfer error: %s",
									&outbuf[i]);

								i += ftc.length;
								bufpt = &outbuf[i];
								toout -= ftc.length;
							}

							/* Read data and store it in the opened file */
							else if (ftc.code == FT_STORE) {
								
								/* Small file */
								if (ftc.length <= toout) {
									if (writen(fdsave, &outbuf[i], ftc.length) != ftc.length) {
										fprintf(stderr, "** SASH Error: writen(): %s\r\n", 
											strerror(errno));
										exit(EXIT_FAILURE);
									}
									
									i += ftc.length;
									bufpt = &outbuf[i];
									toout -= ftc.length;
									break;				
								}
								
								/* Set up progress bar */
								b2send = ftc.length;
								bsent = 0;
								print_pbar(PROGBAR_INIT);	

								/* Flush any bytes allready in buffer */
								if ( (toout > 0) && writen(fdsave, &outbuf[i], toout) != toout) {
									fprintf(stderr, "** SASH Error: writen(): %s\r\n", 
										strerror(errno));
									exit(EXIT_FAILURE);
								}
								else if (toout > 0) {
									bsent = toout;
									print_pbar(PROGBAR_UPDATE);
								}

								winflags |= DONT_SEND_WINZ;
								handle_ftcmd(sock_fd, FT_STORE, fdsave);
								winflags &= ~DONT_SEND_WINZ;
								toout = 0;
							}	
						
							else if (ftc.code == FT_READ) {
								winflags |= DONT_SEND_WINZ;
								handle_ftcmd(sock_fd, FT_READ, fdsave);
								winflags &= ~DONT_SEND_WINZ;
							}
							if (lfile != NULL)
								free(lfile);
							break;
						
						/* Bad command code (?), write everything,
						 * including the magic bytes to stdout */
						default:
							fprintf(stderr, "** SASH Error: Unrecognized command code (0x%02x)\r\n",
								outbuf[i-1]);
							break;
					}

					close(fdsave);
					fdsave = -1;
				}
			}

			/* Write data to stdout (is there any left?) */
            if ( (toout > 0) && writen(STDOUT_FILENO, bufpt, toout) != toout) {
                fprintf(stderr, "** SASH Error: write to stdout: %s\n",
                    strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
	}

	return(0);	
}

/*
 * If code is FT_STORE, data is read from sock_fd and written to fd.
 * If code is FT_READ, data is read from fd and written to sock_fd.
 */
static void
handle_ftcmd(int sock_fd, u_char code, int fd)
{
	u_char *buf;
	u_char encout = 0;	/* Encrypt outgoing / Decrypt incoming */
	struct stat sb;
	int infd;
	int outfd;
	ssize_t rn;

	/* Get size of file */
	if (fstat(fd, &sb) == -1) {
		fprintf(stderr, "\r\n** SASH Error: fstat(): %s\r\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ( (buf = malloc(sb.st_blksize)) == NULL) {
		fprintf(stderr, "\r\n** SASH Error: malloc(): %s\r\n",
			strerror(errno));
		 exit(EXIT_FAILURE);
	}

	if (code == FT_STORE) {
		infd = sock_fd;
		outfd = fd;
	}

	else if (code == FT_READ) {
		infd = fd;
		outfd = sock_fd;
		
		encout++;
		
		/* ACK the file to send */
		send_ftcmd(sock_fd, FT_READ_ACK, sb.st_size, NULL);
		
		/* Set up progressbar */
		bsent = 0;
		b2send = sb.st_size;

		if (b2send != 0)
			print_pbar(PROGBAR_INIT);
		else
			fprintf(stderr, "\r\n** SASH Warning: Source file is empty\r\n");
	}
	
	/* Shouldn't happend */
	else {
		fprintf(stderr, "\r\n** SASH Error: handle_ftcmd(): bad code 0x%x\r\n",
			code);
		exit(EXIT_FAILURE);
	}

	while (bsent < b2send) {
		rn = (b2send - bsent) > sb.st_blksize ? sb.st_blksize: (b2send - bsent);

		if (readn(infd, buf, rn) <= 0) {
			fprintf(stderr, "\r\n** SASH Error: read(): %s\r\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		if (encout) {
			ENCRYPT(buf, rn);
		}
		else {
			DECRYPT(buf, rn);
		}
		
		if (writen(outfd, buf, rn) <= 0) {
			fprintf(stderr, "\r\n** SASH Error: read(): %s\r\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		bsent += rn;
		print_pbar(PROGBAR_UPDATE);
	}

	free(buf);
}

/*
 * Write progressbar for file transfer
 * file - name of file
 * sent - bytes sent
 * tot  - total bytes to send (only used in init/when sent is zero)
 */
void
print_pbar(u_char flag)
{
    static size_t far = 0;         /* Number of bytes to rewind */
	static time_t stime;
	u_char buf[1024];
    ssize_t percnt;
	
	if (flag == PROGBAR_INIT) {
		stime = time(NULL);
		far = 0;
	}

    percnt = 100*((float)bsent / (float)b2send);

    /* Rewind old line */
	memset(buf, '\b', far);
	buf[far] = '\0';
	printf("%s", buf);

    far = snprintf(buf, sizeof(buf), "%s: %u KB of %u (%u%%) [%50s]", tfile, 
		bsent/1024, b2send/1024, percnt, "");

	memset((buf + far)-51, '*', percnt/2);
	printf("%s", buf);

    if (bsent >= b2send) {
		u_int ttime;
		u_int hur;
		u_int min;
		u_int sec;
		
		ttime = time(NULL);
		ttime -= stime;
		hur = ttime / 3600;
		min = ((ttime - (3600*hur)) / 60);
		sec = ttime - (min*60) - (hur*3600);

        printf("\r\n%u bytes transfered in %s%u:%s%u:%s%u (%u bytes/sec)", 
			b2send, (hur > 10) ? "" : "0", hur, (min > 10) ? "" : "0", 
			min, (sec > 10) ? "" : "0", sec, ttime > 0 ? b2send/ttime : b2send);
		far = 0;
    }

	fflush(stdout);
}
