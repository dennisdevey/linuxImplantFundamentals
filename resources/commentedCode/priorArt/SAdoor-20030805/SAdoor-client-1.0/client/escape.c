/*
 *  File: escape.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Connection escape-routines
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
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>
#include <signal.h>
#include "command.h"
#include "sash.h"
#include "command.h"

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

#define ESCAPE_ARGS_MAX		5

struct escargs {
    int argc;
    u_char *argv[ESCAPE_ARGS_MAX];
};

/* Local functions */
static struct escargs *getargs(u_char *, struct escargs *);

/*
 * Escape loop.
 * File transfer, get or put file.
 * Returns a pointer to the path of the local file to transfer,
 * if a such command were issued.
 */
u_char *
escape_loop(int sock_fd, struct termios *stio, int *fdsave)
{
	struct escargs args;
	u_char buf[2*MAXPATHLEN+10];
	u_char *lfile = NULL;
	int fd;

	/* Block signals */
	signal(SIGINT, SIG_IGN);

	/* Restore TTY settings */
	if (tcsetattr(STDIN_FILENO, TCSADRAIN, stio) < 0) {
		fprintf(stderr, "** Error retoring TTY settings: %s\r\n",
			strerror(errno));
		return(NULL);
	}
	
	printf("\n");

	for (;;) {
		printf("%s", ESCAPE_PROMPT);
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1] = '\0';	

		/* Run command locally */
		if (buf[0] == '!') {
			system(&buf[1]);
			continue;
		}

		getargs(buf, &args);

		if (args.argc == 0)
			break;

		/* Get remote file */
		if ((!strncmp(args.argv[0], "get", 4)) && (args.argc == 3)) {
			
			/* Absolut path required */
			if ((args.argv[1][0] != '/') || (args.argv[2][0] != '/')) {
				fprintf(stderr, "** Error: absolute file path required\n");
				continue;
			}
		
			/* Attempt to create local file */
			if ( (fd = open(args.argv[2], O_WRONLY | O_CREAT | O_EXCL, 0600)) == -1) {
				fprintf(stderr, "** Error: open(): %s: %s\n", 
					args.argv[2], strerror(errno));
				continue;
			}

			/* Close and unlink for now */
			if (fdsave == NULL) {
				close(fd);
				unlink(args.argv[2]);
			}
			else
				*fdsave = fd;
			
			/* Send command/request */
			send_ftcmd(sock_fd, FT_GETFILE, 
					strlen(args.argv[1])+1, args.argv[1]);
					
			lfile = strdup(args.argv[2]);
			break;
		}
		
		/* Put local file */
		else if ((!strncmp(args.argv[0], "put", 4)) && (args.argc == 3)) {

			/* Absolut path required */
			if ((args.argv[1][0] != '/') || (args.argv[2][0] != '/')) {
				fprintf(stderr, "** Error: absolute file path required\n");
				continue;
			}

			/* Check for read access on local file */
			if ((fd = open(args.argv[1], O_RDONLY)) == -1) {
				fprintf(stderr, "** Error: open() %s: %s\n",
					args.argv[1], strerror(errno));
				continue;
			}

			/* Close for now */
			if (fdsave == NULL)
				close(fd);
			else
				*fdsave = fd;

			/* Send command/request */
			send_ftcmd(sock_fd, FT_PUTFILE, 
					strlen(args.argv[2])+1, args.argv[2]);
            
			lfile = strdup(args.argv[1]);
			break;
		}

		else if (!strncmp(args.argv[0], "quit", 5))
			exit(EXIT_SUCCESS);
		
		else {
			printf("\n");
			printf("Available commands (enter to exit):\n");
			printf("  ! command                  - Run command locally\n");
			printf("  get remote-file local-file - Get remote file\n");
			printf("  put local-file remote-file - Put local file\n");
			printf("  quit                       - Exit sash\n");
			printf("\n");
			continue;
		}
	}

	/* Set TTY back to raw mode */
	if (tty_raw(STDIN_FILENO, stio) < 0) {
		fprintf(stderr, "** SAsh Error: tty_raw() error\n");
			exit(EXIT_FAILURE);
	}

	/* Get back real prompt */
	buf[0] = '\n';
	ENCRYPT(&buf[0], 1);
	
	if (write(sock_fd, &buf[0], 1) <= 0) {
		fprintf(stderr, "** SAsh Error: write():%s\n",
			strerror(errno));
	}

	return(lfile);
}

/*
 * Send file transfer command header.
 * dlen should be host endian.
 * Data is not copyed if it's a NULL pointer.
 */
void
send_ftcmd(int sock_fd, u_char code, u_int dlen, u_char *data)
{
    u_char outbuf[1024];
    size_t i;
    struct ftcmd ftc;

    if ((data != NULL) && 
			 ((dlen + MAGIC_BYTES_LEN + sizeof(ftc)) > sizeof(outbuf))) {
        fprintf(stderr, "** SASH Error: File tranfer command  is "
			"larger than buffer");
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
		fprintf(stderr, "** SASH Error: writen(): %s\n", 
			strerror(errno));
        exit(EXIT_FAILURE);
    }
}


/*
 * Lexer for argument string.
 */
#define ISSPACE(c)        (((c) == ' ') || ((c) == '\t'))
static struct escargs *
getargs(u_char *str, struct escargs *args)
{
    u_char *pt = str;
    int i = 0;

	if (str == NULL) {
		args->argc = 0;
		args->argv[0] = (u_char *)NULL;
		return(args);
	}

    while (*pt && (i < (ESCAPE_ARGS_MAX - 1)) ) {

        args->argv[i++] = pt;

        while(!ISSPACE(*pt) && *pt) 
			pt++;

        if (*pt == '\0')
            break;

        *pt = '\0';
        pt++;

        while (ISSPACE(*pt))
			pt++;
    }

    args->argv[i] = (u_char *)NULL;
    args->argc = i;
    return(args);
}

