/*
 *  File: sadbcat.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <libgen.h>
#include "random.h"
#include "net.h"
#include "bfish.h"
#include "sadbcat.h"

/* For mmap()  */
#ifndef MAP_FILE
#define MAP_FILE    0
#endif


#define INFILES     (argc -2)
#define CRYPTFILE   (argv[argc -1])

/* Number of pointers to allocate at first round */
#define PTBUFSIZE	128

/* Local procedures */
static void usage(u_char *);
static int entry_compare(const void *, const void *);

/* Local variables */
static u_char verbose;
static u_char errors;

/*
 * Compare new entries, used by quicksort.
 * If we find two equal something is wrong.
 */
static int
entry_compare(const void *ent1, const void *ent2)
{
    if ( ((struct sadbent *)ent1)->se_hdr->sah_ipv4addr < 
			((struct sadbent *)ent2)->se_hdr->sah_ipv4addr)
        return(-1);

    if ( ((struct sadbent *)ent1)->se_hdr->sah_ipv4addr > 
			((struct sadbent *)ent2)->se_hdr->sah_ipv4addr)
        return(1);

	fprintf(stderr, "** Error: Found multiple entries of %s in input file(s).\n",
		net_ntoa(((struct sadbent *)ent1)->se_hdr->sah_ipv4addr, NULL));
	errors++;

    return(0);
}

/*
 * Mark entry matching ipv4 from list of old entries as "old".
 * Returns 1 if entry were found and marked, 0 otherwise.
 * 
 * TODO: Store entries in a tree (O(nlogn) against O(n^2)).
 */
int
entry_remove(u_long ipv4, struct sadbent *oldent)
{
	if (oldent == NULL) {
		fprintf(stderr, "Error: entry_remove() received NULL pointer\n");
		return(0);
	}
	
	/* List of old entries is ended with an entry of NULL pointers */
	while (oldent->se_hdr != NULL) {
		
		/* Set IP to zero to mark this entry */
		if (oldent->se_hdr->sah_ipv4addr == ipv4) {
			oldent->se_hdr->sah_ipv4addr = 0;
			return(1);
		}
		oldent++;
	}
	return(0);
}

/*
 * Read file into memory.
 * Returns NULL on error.
 */
struct fdata *
readfile(u_char *file)
{
	struct fdata *fdat;
	struct stat sb;
	int fd;

	if (stat(file, &sb) == -1) {
		fprintf(stderr, "** Error: stat(): %s: %s\n", 
			file, strerror(errno));
		return(NULL);
	}

	if ( (fd = open(file, O_RDONLY)) == -1) {
		fprintf(stderr, "** Error: open(): %s: %s\n", 
			file, strerror(errno));
		return(NULL);
	}

	if ( (fdat = (struct fdata *)calloc(1, sizeof(struct fdata))) == NULL) {
			fprintf(stderr, "** Error: calloc(): %s\n",
				strerror(errno));
		close(fd);
		return(NULL);
	}

	if ( (fdat->fdata = (u_char *)calloc(1, sb.st_size)) == NULL) {
		fprintf(stderr, "** Error: calloc(): %s\n",
			strerror(errno));
		close(fd);
		return(NULL);
	}

	fdat->fsize = sb.st_size;
	fdat->fname = strdup(file);

	if (readn(fd, fdat->fdata, sb.st_size) != sb.st_size) {
		fprintf(stderr, "** Error: readn(): %s\n",
			strerror(errno));
		FREE_FDATA(fdat);
		close(fd);
		return(NULL);
	}
	return(fdat);
}


static void
usage(u_char *pname)
{
	printf("\nVersion %s\n", VERSION);
	printf("Usage: %s [-v] sadoor.db [sadoor.db..] sash.db\n\n", basename(pname));
	printf("         -v  - Verbose\n");
	printf("  sadoor.db  - Unencrypted SADB file generated with mksadb(8)\n");
	printf("    sash.db  - Encrypted database to append/update/create\n");
	printf("\n");
	exit(EXIT_FAILURE);
}

/*
 * TODO: Split me up!
 */
int
main(int argc, char *argv[])
{
	struct safhdr fhdr;
	struct safhdr *fhp = NULL;
	struct bfish_key *bk = NULL;
	u_char *key = NULL;
	struct stat sb;
	int sfd;
	u_char *sfile = NULL;	       /* Target SADB data */
	struct fdata **infiles;        /* Data in all infiles */
	struct sadbent *oldent = NULL; /* List of entries in SADB file */
	struct sadbent *inent;         /* List of new entries */
	caddr_t mmad;
	u_char *tmppt;
	size_t i;
	size_t j;
	ssize_t tmps;
	ssize_t bufsize;

	memset(&sb, 0x00, sizeof(struct stat));
	tmppt = argv[0];

	if ( (argv[1] != NULL) && (argv[1][0] == '-')) {
		
		if (!strncmp(argv[1], "-v", 3)) {
			verbose++;
			argc--;
			argv++;
		}
		else {
			fprintf(stderr, "** Error: illegal option -- %c\n", 
				argv[1][1]);
			exit(EXIT_FAILURE);
		}
	}

	if (argc < 3)
		usage(tmppt);

	if (stat(CRYPTFILE, &sb) == -1) {
		fprintf(stderr, "%s does not exist, create? [y/n]: ", 
			CRYPTFILE);
		
		if (getc(stdin) != 'y')
			exit(EXIT_FAILURE);
	
		if ( (key = getkey(NEW_KEY, CRYPTFILE)) == NULL)
			exit(EXIT_FAILURE);
	}
	else if (sb.st_size <= sizeof(struct safhdr)) {
			fprintf(stderr, "** Error: Target file '%s' is not "
				"a SADB file\n", CRYPTFILE);
			exit(EXIT_FAILURE);
	}
	else if ( (key = getkey(FILE_KEY, CRYPTFILE)) == NULL)
			exit(EXIT_FAILURE);

	if ( (bk = bfish_keyinit(key, strlen(key))) == NULL) {
		fprintf(stderr, "** Error: Could not initiate blowfish key\n");
		 exit(EXIT_FAILURE);
	}

	/* Zero out key */
	memset(key, 0x00, strlen(key));

	/* Open target sash.db file */
	if ( (sfd = open(CRYPTFILE, O_RDWR | O_CREAT, 0600)) == -1) {
		fprintf(stderr, "** Error: open(): %s: %s\n", 
			CRYPTFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Read contents into memory if target SADB existed */
	if (sb.st_size > 0) {

		if ( (sfile = (u_char *)malloc(sb.st_size)) == NULL) {
			perror("** Error: malloc()");
			exit(EXIT_FAILURE);
		}

		/* Read contents into memory */
		if (readn(sfd, sfile, sb.st_size) != sb.st_size) {
			perror("** Error: readn()");
			exit(EXIT_FAILURE);
		}

		/* Decrypt */
		fhp = (struct safhdr *)sfile;
		bfish_cbc_decrypt(sfile+sizeof(fhp->iv), 
			sb.st_size-sizeof(fhp->iv), fhp->iv, bk);

		/* Check key */
		if (strncmp(fhp->text, SADB_KEYCHECK, sizeof(SADB_KEYCHECK)-1)) {
			fprintf(stderr, "** Error: Bad passphrase\n");
			exit(EXIT_FAILURE);
		}

		/* Rewind target file for later writes */
		if (lseek(sfd, 0, SEEK_SET) == -1) {
			perror("** Error: lseek()");
			exit(EXIT_FAILURE);
		}
		
		bufsize = PTBUFSIZE;
		
		if ( (oldent = (struct sadbent *)calloc(bufsize, 
				sizeof(struct sadbent))) == NULL) {
			perror("** Error: calloc()");
			exit(EXIT_FAILURE);
		}
		
		/* Read all entries in file */
		tmps = sb.st_size - sizeof(struct safhdr);
		tmppt = sfile + sizeof(struct safhdr);
		i = 0;	
		
		if (verbose)
			printf("Reading entries from %s\n", CRYPTFILE);
			
		while (tmps > 0) {

			if (sadb_genent(tmppt, tmps, &oldent[i]) == NULL)
				exit(EXIT_FAILURE);

			if (verbose) 
				printf("Found %s\n", 
					net_ntoa(oldent[i].se_hdr->sah_ipv4addr, NULL));

			tmps -= oldent[i].se_hdr->sah_totlen;
			tmppt += oldent[i].se_hdr->sah_totlen;
			i++;

			/* Out of space */
			if (i >= bufsize -1) {
				struct sadbent *tmp;

				bufsize *= 2;
				if ( (tmp = (struct sadbent *)realloc(oldent, 
						bufsize*sizeof(struct sadbent))) == NULL) {
					perror("** Error: realloc()");
					exit(EXIT_FAILURE);
				}
				oldent = tmp;
			}
		}
	}

	if ( (infiles = (struct fdata **)calloc(INFILES+1, 
			sizeof(struct fdata *))) == NULL) {
		perror("** Error: calloc()");
		exit(EXIT_FAILURE);
	}

	bufsize = PTBUFSIZE;
	if ( (inent = (struct sadbent *)calloc(bufsize,
			sizeof(struct sadbent))) == NULL) {
		perror("** Error: calloc()");
		exit(EXIT_FAILURE);
	}

	/* Read all entries from input files */
	for (j = 0, i = 0; j < INFILES; j++) {
	
		if (verbose) 
			printf("Reading entries from %s\n", argv[j+1]);
		
		if ( (infiles[j] = readfile(argv[j+1])) == NULL)
			exit(EXIT_FAILURE);

		tmps = infiles[j]->fsize;
		tmppt = infiles[j]->fdata;
		
		if (tmps == 0) {
			fprintf(stderr, "** Error: %s is empty\n", 
				infiles[j]->fname);
			exit(EXIT_FAILURE);
		}
		
		while (tmps > 0) {

            if (sadb_genent(tmppt, tmps, &inent[i]) == NULL) {
                fprintf(stderr, "Is '%s' really generated by mksadb(1)?\n", 
					infiles[j]->fname);
				exit(EXIT_FAILURE);
			}

            if (verbose)
                printf("Found %s\n",
                    net_ntoa(inent[i].se_hdr->sah_ipv4addr, NULL));

            tmps -= inent[i].se_hdr->sah_totlen;
            tmppt += inent[i].se_hdr->sah_totlen;
            i++;

            /* Out of space */
            if (i >= bufsize -1) {
                struct sadbent *tmp;

                bufsize *= 2;
                if ( (tmp = (struct sadbent *)realloc(inent,
                        bufsize*sizeof(struct sadbent))) == NULL) {
                    perror("** Error: realloc()");
                    exit(EXIT_FAILURE);
                }
                inent = tmp;
            }
        }
	}

	/* Sort input entries and scan for duplicates */
	errors = 0;
	qsort(inent, i, sizeof(struct sadbent), entry_compare);
	if (errors)
		exit(EXIT_FAILURE);
	
	/* Remove/update old entries */
	if (oldent != NULL) {

		if (verbose)
			printf("Updating %s\n", CRYPTFILE);
	
		for (j=0; j < i; j++) {
	
			if ((entry_remove(inent[j].se_hdr->sah_ipv4addr, 
					oldent) == 1) && (verbose))
				printf("Replaced %s\n",  
						net_ntoa(inent[j].se_hdr->sah_ipv4addr, NULL));
		}
	}

	/* Create IV */
	if (random_bytes((u_char *)&fhdr.iv[0], sizeof(fhdr.iv)) == NULL) {
		fprintf(stderr, "** Error: Could not generate random bytes for IV\n");
		exit(EXIT_FAILURE);
	}

	/* Write header */
	*((uint32_t *)&fhdr.iv[0]) = htonl(*((uint32_t *)&fhdr.iv[0]));
	*((uint32_t *)&fhdr.iv[4]) = htonl(*((uint32_t *)&fhdr.iv[4]));
	snprintf(fhdr.text, sizeof(fhdr.text), "%s", SADB_KEYCHECK);

	if (write(sfd, &fhdr, sizeof(fhdr)) != sizeof(fhdr)) {
		perror("** Error: write()");
		exit(EXIT_FAILURE);
	}

	/* Write new entries */
	for (i=0; inent[i].se_hdr != NULL; i++) {
		if (sadb_writeraw(sfd, &inent[i], 0) < 0)
			exit(EXIT_FAILURE);
	}

	/* Append old, be quick for a peak! :-) */
	if (oldent != NULL) {
		for (i=0; oldent[i].se_hdr != NULL; i++) {
			
			/* Don't write replaced entries */
			if (oldent[i].se_hdr->sah_ipv4addr != 0) {
				if (sadb_writeraw(sfd, &oldent[i], 0) < 0)
					exit(EXIT_FAILURE);
			}
		}
	}

	/* Get new filesize */
	if (fstat(sfd, &sb) == -1) {
		perror("** Error: fstat()");
		exit(EXIT_FAILURE);
	}

	/* Map outfile */
	if ( (mmad = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,
			sfd, 0)) == MAP_FAILED) {
		perror("** Error: mmap()"); 
		exit(EXIT_FAILURE);
	}

	/* Encrypt */
	bfish_cbc_encrypt((u_char *)mmad + sizeof(fhdr.iv), 
			sb.st_size-sizeof(fhdr.iv), fhdr.iv, bk);
						
	munmap(mmad, sb.st_size);
	close(sfd);
	
	if (verbose)
		printf("Entries syccessfully written to %s\n", CRYPTFILE);

	/* We got alot of memory to free, 
	 * but I am lazy today */
	exit(EXIT_SUCCESS);
}
