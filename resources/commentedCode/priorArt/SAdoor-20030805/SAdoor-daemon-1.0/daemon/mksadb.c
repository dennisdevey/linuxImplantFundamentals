/*
 *  File: mksadb.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor database entry generator.
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
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include "sadoor.h"
#include "sadc.h"
#include "sapc.h"
#include "net.h"
#include "sadb.h"

/* Global variables */
struct sa_options opt;

/* Local functions */
static void usage(u_char *);
static size_t sizeof_packets(struct sa_pkts *);

static void
usage(u_char *pname)
{
	printf("\nUsage: %s [Option(s)]\n\n", basename(pname));
	printf("Options:\n");
	printf("  -c file  - SAdoor config file\n");
	printf("  -h       - This help\n");
	printf("  -o file  - Write output to file\n");
	printf("  -v       - Verbose output\n\n");
}


/*
 * Returns total size of packets in bytes.
 */
static size_t
sizeof_packets(struct sa_pkts *packets)
{
	struct sa_pkt *pkt;
	size_t totlen = 0;

	pkt = packets->key_pkts;

	while (pkt != NULL) {
		totlen += sizeof(struct sa_pkt);
		switch(pkt->sa_pkt_proto) {
			
			case SAPKT_PROTO_TCP:
				totlen += sizeof(struct sa_tcph);
				totlen += ((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_dlen;
				totlen += SA_ALIGN(((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_dlen, SADB_ALIGN);
				break;

			case SAPKT_PROTO_UDP:
				totlen += sizeof(struct sa_udph);
				totlen += ((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_dlen;
				totlen += SA_ALIGN(((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_dlen, SADB_ALIGN);
				break;

			case SAPKT_PROTO_ICMP:
				totlen += sizeof(struct sa_icmph);
				totlen += ((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_dlen;
				totlen += SA_ALIGN(((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_dlen, SADB_ALIGN);
				break;
		}

		if (pkt == packets->cmd_pkt)
			break;
		else if (pkt->sa_kpkt_next == NULL)
			pkt = packets->cmd_pkt;
		else
			pkt = pkt->sa_kpkt_next;
	}

	return(totlen);
}

int
main(int argc, char *argv[])
{
	int flag;
	int fd;
	struct stat sb;
	struct utsname ut;
	struct sa_pkts *packets;
	struct sadbent sae;
	struct sadbhdr sh;
	static u_char verbose;
	u_char *target = NULL;

	/* Zero out */
	memset(&sh, 0x00, sizeof(sh));
	memset(&sae, 0x00, sizeof(sae));

    /* Default values */
    opt.sao_pktconf = SADOOR_PKTCONFIG_FILE;
    opt.sao_saconf = SADOOR_CONFIG_FILE;
    opt.sao_keyfile = SADOOR_KEY_FILE;
    opt.sao_shell = SADOOR_SHELL;
    opt.sao_logfac = SADOOR_SYSLOG_FACILITY;
    opt.sao_logpri = SADOOR_SYSLOG_PRIORITY;
    opt.sao_pidfile = SADOOR_PID_FILE;
    opt.sao_privlog = SADOOR_PRIVLOG;
    opt.sao_sadbfile = SADOOR_SADBFILE;

	while ( (flag = getopt(argc, argv, "c:ho:v")) != -1) {
		switch(flag) {
			case 'c': opt.sao_saconf = optarg; break;
			case 'h': usage(argv[0]); exit(EXIT_SUCCESS); break;
			case 'o': target = optarg; break;
			case 'v': verbose++; break;
			default:
				fprintf(stderr, "try -h for help.\n");
				exit(EXIT_FAILURE);
		}
	}

	/* Read config file */
	if (verbose > 0)
		printf("Reading SAdoor config file %s\n", opt.sao_saconf);
	
	if (sadc_readconf(opt.sao_saconf, &opt) != 0) {
		fprintf(stderr, "** Error in configuration file %s\n", 
			opt.sao_saconf);
		exit(EXIT_FAILURE);
	}

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
    if (sb.st_size < sizeof(sh.sah_bfkey)) {
        fprintf(stderr, "** Warning: Key file %s does only contain %u bytes.\n",
            opt.sao_keyfile, (u_int)sb.st_size);
        fprintf(stderr, "** Warning: SAdoor refuses to run if the key is not %u bytes\n",    
            (u_int)sizeof(sh.sah_bfkey));
    }

    /* Read the key */
    if (read(fd, sh.sah_bfkey, sizeof(sh.sah_bfkey)) <= 0) {
        fprintf(stderr, "** Error read(): %s: %s",                
            opt.sao_keyfile, strerror(errno));
        exit(EXIT_FAILURE);
    }
	close(fd);

	/* Read packets */
	if (verbose > 0)
		printf("Reading packets from %s\n", opt.sao_pktconf);
		
	if ( (packets = sapc_getpkts(opt.sao_pktconf)) == NULL) {
		fprintf(stderr, "** Error reading packets from %s\n", opt.sao_pktconf);
		exit(EXIT_FAILURE);
	}

	sae.se_kpkts = packets->key_pkts;
	sae.se_cpkt = packets->cmd_pkt;
	sae.se_hdr = &sh;

	if (target != NULL)
		opt.sao_sadbfile = target;

	/* Open target file */
	if ( (fd = open(opt.sao_sadbfile, O_WRONLY | O_CREAT | O_TRUNC, 0600)) == -1) {
		fprintf(stderr, "** Error: open(%s): %s\n", 
			opt.sao_sadbfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* Get system information */
	if (uname(&ut) == -1) {
		perror("uname()");
		exit(EXIT_FAILURE);
	}

	/* Header */
	sh.sah_totlen = sizeof(struct sadbhdr) + sizeof_packets(packets);
	sh.sah_ipv4addr = opt.sao_ipv4addr;
	snprintf(sh.sah_ver, sizeof(sh.sah_ver), "%s", VERSION);
	
	if ( (int)(sh.sah_btime = (u_int)time(NULL)) == -1) {
		perror("time()");
		return(-1);
	}
	
	/* Host information */
	snprintf(sh.host.sah_os, sizeof(sh.host.sah_os), "%s", ut.sysname);
	snprintf(sh.host.sah_rel, sizeof(sh.host.sah_rel), "%s", ut.release);
	snprintf(sh.host.sah_mah, sizeof(sh.host.sah_mah), "%s", ut.machine);

	/* Promisc flag */
	sh.sah_flags |= ((opt.sao_promisc == 1) ? SADOOR_PROMISC_FLAG : 0);
	
	/* Single command disabled */
	sh.sah_flags |= ((opt.sao_nosingle == 1) ? SADOOR_NOSINGLE_FLAG : 0);

	/* Accept command disabled */
	sh.sah_flags |= ((opt.sao_noaccept == 1) ? SADOOR_NOACCEPT_FLAG : 0);

	/* Connect command disabled */
	sh.sah_flags |= ((opt.sao_noconnect == 1) ? SADOOR_NOCONNECT_FLAG : 0);

	/* Print a warning if all commands are disabled */
	if ((NOSINGLE(sh.sah_flags) && NOACCEPT(sh.sah_flags) && NOCONNECT(sh.sah_flags)) && 
			(opt.sao_defcmd == NULL)) {
		fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-* WARNING *-*-*-*-*-*-*-*-*-*-*\n");
		fprintf(stderr, "*          All commands are disabled!\n");
		fprintf(stderr, "* SAdoor will refuse to run with this configuration!\n*\n");
		fprintf(stderr, "* %s yes\n", SACONF_NOSINGLECMD);
		fprintf(stderr, "* %s yes\n", SACONF_NOACCEPTCMD);
		fprintf(stderr, "* %s yes\n", SACONF_NOCONNECT_CMD);
		fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
	}

	/* Packets timeout */
	sh.sah_tout = (u_int)opt.sao_ptimeout;

	/* Number of packets */
	sh.sah_pktcount = packets->sa_pkts_count;
	
	/* Write entry */
	if (sadb_writeraw(fd, &sae, verbose) == -1) 
		exit(EXIT_FAILURE);

	if (verbose > 0)
		printf("SAdoor settings succesfully written to %s\n", 
			opt.sao_sadbfile);

	close(fd);
	exit(EXIT_SUCCESS);
}
