/*
 *  File: sadump.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor database management routines
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/param.h>
#include "bfish.h"
#include "utils.h"
#include "net.h"
#include "sadb.h"
#include "sapc.h"
#include "sash.h"
#include "version.h"

/* Local options */
static struct dumpopt {
	u_char opt_nocrypt:1;   /* Target SADB file is not encrypted */
	u_char opt_raw:1;		/* Write output in raw */
	u_char opt_hname:1;		/* Translate IP addresses into hostnames */
	u_char opt_ppkts:1;		/* Supress printing of packets flag */
	u_char opt_pkey:1;      /* Print host-key */
	u_char *opt_sadb;		/* Target database file */
	u_int opt_tip;			/* Requested entry */
	u_int opt_ipa;			/* Network ordered IP of requested entry */
} dop;

static void
usage(u_char *pname)
{
	printf("\nVersion %s\n", VERSION);
	printf("Usage: %s [host] [Option(s)]\n\n", basename(pname));
	printf("Options:\n");
	printf("    -d       - SADB file is not encrypted\n");
	printf("    -f file  - SADB file (default: ~/%s/%s)\n", SASH_CONFIG_DIR, SASH_SADB_FILE);
	printf("    -h       - This help\n");
	printf("    -k       - Print private host-key (hex)\n");
	printf("    -n       - Do not resolve hostnames\n");
	printf("    -p       - Supress printing of packets\n");
	printf("    -r       - Print raw entry (all fields)\n\n");
}

int
main(int argc, char *argv[])
{
	u_char abspath[MAXPATHLEN+1];
	u_char *home;
	int flag;
	struct mfile *mf;
	struct mfile mftmp;
	struct sadbent sae;

	memset(&dop, 0x00, sizeof(struct dumpopt));
	memset(&sae, 0x00, sizeof(struct sadbent));

	if ((argc > 1) && (argv[1][0] != '-')) {
		if ( (int)(dop.opt_tip = net_inetaddr(argv[1])) == -1) {
			fprintf(stderr, "** Error: Could not resolve target IP/host \"%s\"\n", 
				argv[1]);
			exit(EXIT_FAILURE);
		}
		argv++;
		argc--;
	}

	/* Resolve hostname and print packets by default */
	dop.opt_hname = 1;
	dop.opt_ppkts = 1;

	while ( (flag = getopt(argc, argv, "dprhf:nk")) != -1) {
		switch(flag) {
			case 'd': dop.opt_nocrypt = 1; break;
			case 'p': dop.opt_ppkts = 0; break;
			case 'r': dop.opt_raw = 1; break; 
			case 'f': dop.opt_sadb = optarg; break;
			case 'h': usage(argv[0]); exit(EXIT_SUCCESS); break;	
			case 'k': dop.opt_pkey =1; break;
			case 'n': dop.opt_hname = 0; break;
			default:
				fprintf(stderr, "try -h for help.\n");
				exit(EXIT_FAILURE);
		}
	}

	/* Look for sadb file in default directory */
	if (dop.opt_sadb == NULL) {
		
		if ( (home = getenv("HOME")) == NULL) {
			fprintf(stderr, "** Error: environment variable \"HOME\" is not set.\n");
			exit(EXIT_FAILURE);
		}

		snprintf(abspath, sizeof(abspath)-1, "%s/%s/%s", 
			home, SASH_CONFIG_DIR, SASH_SADB_FILE);
		dop.opt_sadb = abspath;
	}

	if ( (mf = open_mfile(dop.opt_sadb)) == NULL)
		exit(EXIT_FAILURE);	

	if (mf->mf_size <=  sizeof(struct safhdr)) {
		fprintf(stderr, "** Error: '%s' is not "
			"a SADB file\n", dop.opt_sadb);
		exit(EXIT_FAILURE);
	}

	/* Copy file structure in case of decryption */
	memcpy(&mftmp, mf, sizeof(struct mfile));

	/* Decrypt file */
	if (!dop.opt_nocrypt) {
		struct bfish_key *bk;
		struct safhdr *fhp;
		u_char *key;
		
		if ( (key = getkey(FILE_KEY, dop.opt_sadb)) == NULL)
			exit(EXIT_FAILURE);
		
		if ( (bk = bfish_keyinit(key, strlen(key))) == NULL) {
			fprintf(stderr, "** Error: Could not initiate blowfish key\n");
			exit(EXIT_FAILURE);
		}

		/* Zero out key */
		memset(key, 0x00, strlen(key));

		/* Decrypt */
		fhp = (struct safhdr *)mf->mf_file;
		bfish_cbc_decrypt(mf->mf_file+sizeof(fhp->iv),
			mf->mf_size-sizeof(fhp->iv), fhp->iv, bk);

		/* Zero out key */
		memset(bk, 0x00, sizeof(struct bfish_key));
		free(bk);

		/* Check key */
		if (strncmp(fhp->text, SADB_KEYCHECK, sizeof(SADB_KEYCHECK)-1)) {
			fprintf(stderr, "** Error: Bad passphrase\n");
			exit(EXIT_FAILURE);
		}

		mftmp.mf_size -= sizeof(struct safhdr);
		mftmp.mf_file += sizeof(struct safhdr);
	}

	/* Print requested entry */
	if (dop.opt_tip != 0) {
		if (sadb_getentip(&mftmp, dop.opt_tip, &sae) == NULL)
			exit(EXIT_FAILURE);
	
		/* Write raw */
		if (dop.opt_raw == 1) {
			if (sadb_writeraw(STDOUT_FILENO, &sae, 0) == -1)
				exit(EXIT_FAILURE);
		}
		else	
			sadb_printent(&sae, dop.opt_hname, dop.opt_ppkts, dop.opt_pkey);
	}
	/* Print all entries in file */
	else {
		while (mftmp.mf_size > 0) {
			if (sadb_genent(mftmp.mf_file, mftmp.mf_size, &sae) == NULL)
				exit(EXIT_FAILURE);
				
			mftmp.mf_size -= sae.se_hdr->sah_totlen;
			mftmp.mf_file += sae.se_hdr->sah_totlen;

			if (dop.opt_raw == 1) {
				if (sadb_writeraw(STDOUT_FILENO, &sae, 0) == -1) 
					exit(EXIT_FAILURE);
			}
			else
				sadb_printent(&sae, dop.opt_hname, dop.opt_ppkts, dop.opt_pkey);
		}
	}

	close_mfile(mf);
	exit(EXIT_SUCCESS);
}
