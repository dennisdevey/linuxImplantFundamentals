/*
 *  File: sashcfg.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor client configuration file routines
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
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "net.h"
#include "utils.h"
#include "sashcfg.h"
#include "sash.h"

/* Number of pointers for lexer to allocate at start */
#define INIT_BUFSIZE    512

/* Used by sashcfg_setnext() only */
#define MATCH_OR_DIE(a, b) \
    if ((a) == NULL || (b) == NULL || (strcmp((a), (b)) != 0)) { \
        fprintf(stderr, "Syntax Error (entry %s): expected '%s', received '%s'\n", \
            ent, (b), (a)); \
        return(1); \
    }

/* Local variables */
u_char verbose;

/* Local procedures */
static int sashcfg_setnext(u_char ***, struct sashopt *);
static u_char **sashcfg_lexer(u_char *);
static void parse_tcpflags(u_char ***, u_char *, struct sashopt *);


/*
 * Fill in global options in entry pointed to by opts,
 * by the values set in entry in config file with the same 
 * IP address as pointed to by opts->sa_tip.
 * Returns -1 on error, 1 if no entry mathing opts->sa_tip
 * were found and 0 on success.
 */
int
sashcfg_setopt(struct sashopt *opts)
{
	struct sashopt tmp;	
	u_char **arr;
	u_char **arrpt;
	struct mfile *mf;
	int ret;

	if (opts == NULL) {
		fprintf(stderr, "** Error: sash_setopt() received "
			"NULL pointer as entry\n");
		return(-1);
	}
	
	/* Set verbose level */
	verbose = opts->sa_verbose;

	if ( (mf = open_mfile(opts->sa_sacfg)) == NULL)
		return(-1);

	if ( (arr = arrpt = sashcfg_lexer(mf->mf_file)) == NULL) {
		close_mfile(mf);
		return(-1);
	}

	/* Loop through all entries until we find a match */
	memcpy(&tmp, opts, sizeof(struct sashopt));
	while ( (ret = sashcfg_setnext(&arrpt, &tmp)) == 0) {
		
		/* Found entry */
		if (tmp.sa_tip == opts->sa_tip) {
			
			if (opts->sa_verbose > 1)
				printf("Found entry\n");
			
			memcpy(opts, &tmp, sizeof(struct sashopt));
			break;
		}

		memcpy(&tmp, opts, sizeof(struct sashopt));
	}

	/* Free strndup'ed strings */
	if (arr != NULL) {
		u_char **tmp = arr;
		
		while (*tmp != NULL)
			free(*(tmp++));
		
		free(arr);
	}
	
	if (mf != NULL)
		close_mfile(mf);

	if (ret == 1) {
		fprintf(stderr, "** Error reading config file %s\n", 
			opts->sa_sacfg);
		return(-1);
	}

	return((ret == EOF) ? 1 : ret);
}

/*
 * Parse and set the values for next entry in file 
 * in the options structure pointed to by opt.
 * Returns EOF on end of file (or end of array ..),
 * 1 on error and 0 on success.
 */
static int
sashcfg_setnext(u_char ***arr, struct sashopt *opt)
{
	u_char *ent;		/* Name of current entry */
	u_long entfields = 0; /* Entry fields altered */
	u_long tmp;
	u_long err = 0;
	
	if (*(*arr) == NULL)
		return(EOF);

	
	/* IP address expected */
	if (!isip(*(*arr))) {
		fprintf(stderr, "Syntax error: '%s': Is not an IP address\n", *(*arr));
		return(1);
	}
	
	opt->sa_tip = net_inetaddr(*(*arr));
	ent = *(*arr);
	(*arr)++;
	
	/* Next up is a block begin */
	MATCH_OR_DIE(*(*arr), CFG_BLOCK_BEGIN);
	(*arr)++;
	
	while ((*(*arr) != NULL) && strcmp(*(*arr), CFG_BLOCK_END)) {
		u_char typerr = 0;
		u_long field;		/* Current entry */		

		if (CMATCH(*(*arr), BASEADDR_TOK)) 
			field = BASEADDR_FLAG;			
	
		else if (CMATCH(*(*arr), SADB_TOK)) 
			field = SADB_FLAG;

        else if (CMATCH(*(*arr), TIMEOUT_TOK)) 
			field = TIMEOUT_FLAG;

        else if (CMATCH(*(*arr), DELAYMS_TOK)) 
			field = DELAYMS_FLAG;

        else if (CMATCH(*(*arr), ESCCHR_TOK)) 
			field = ESCCHR_FLAG;

        else if (CMATCH(*(*arr), SADDR_TOK)) 
			field = SADDR_FLAG;

        else if (CMATCH(*(*arr), IPTOS_TOK)) 
			field = IPTOS_FLAG;
		
        else if (CMATCH(*(*arr), IPID_TOK)) 
			field = IPID_FLAG;

        else if (CMATCH(*(*arr), IPTTL_TOK)) 
			field = IPTTL_FLAG;

        else if (CMATCH(*(*arr), ADDTTL_TOK)) 
			field = ADDTTL_FLAG;
	
        else if (CMATCH(*(*arr), TCPACK_TOK)) 
			field = TCPACK_FLAG;

        else if (CMATCH(*(*arr), TCPSEQ_TOK)) 
			field = TCPSEQ_FLAG;

        else if (CMATCH(*(*arr), TCPFLAGS_TOK)) 
			field = TCPFLAGS_FLAG;

        else if (CMATCH(*(*arr), UDPSPORT_TOK)) 
			field = UDPSPORT_FLAG;

        else if (CMATCH(*(*arr), UDPDPORT_TOK)) 
			field = UDPDPORT_FLAG;

        else if (CMATCH(*(*arr), TCPSPORT_TOK)) 
			field = TCPSPORT_FLAG;

        else if (CMATCH(*(*arr), TCPDPORT_TOK)) 
			field = TCPDPORT_FLAG;

        else if (CMATCH(*(*arr), TCPWIN_TOK)) 
			field = TCPWIN_FLAG;

		else if (CMATCH(*(*arr), ICMPID_TOK))
			field = ICMPID_FLAG;
		
		else if (CMATCH(*(*arr), ICMPSEQ_TOK))
			field = ICMPSEQ_FLAG;
			
		else {
			fprintf(stderr, "Syntax error (entry %s): Unrecognized keyword: '%s'\n", 
				ent, *(*arr));
			return(1);
		}

		if (field & entfields) {
			fprintf(stderr, "Error (entry %s): multiple assignments for '%s'\n",
				ent, *(*arr));
			return(1);
		}
		entfields |= field;
		
		(*arr)++;
		MATCH_OR_DIE(*(*arr), CFG_ASSIGN);
		(*arr)++;

		/* TCP flags can be almost anything .. */
		if (field == TCPFLAGS_FLAG) 
			parse_tcpflags(arr, &typerr, opt);
	
		/* Set random flag */
		else if (CMATCH(*(*arr), RANDOM_TOK)) {
			
			/* These fields can't be randomized */
			switch(field) {
				case BASEADDR_FLAG:
				case SADB_FLAG:
				case TIMEOUT_FLAG:
				case DELAYMS_FLAG:
				case ESCCHR_FLAG:
				case ADDTTL_FLAG:
				case TCPWIN_FLAG:
					typerr++;
					break;
				
				/* Set randomize flag for field */
				default:
					opt->sa_randflags |= field;
					break;
			}
		}
		/* Numeric value */
		else if (strisnum(*(*arr), &tmp)) {

			switch (field) {
				
				case ESCCHR_FLAG:
					if (!ISu16(tmp))
						typerr++;
					else
						opt->sa_esc = CONTROL(tmp);
					break;
					
				case IPTOS_FLAG:
					if (!ISu8(tmp))
						typerr++;
					else
						opt->sa_iptos = (u_char)tmp;
					break;
					
				case TIMEOUT_FLAG:
					opt->sa_ctout = (time_t)tmp;
					break;

				case DELAYMS_FLAG:
					opt->sa_tdelay = (time_t)tmp;
					break;
					
				case IPID_FLAG:
					if (!ISu16(tmp))
						typerr++;
					else
						opt->sa_ipid = htons((u_short)tmp);
					break;
					
				case IPTTL_FLAG:
					if (!ISu8(tmp))
						typerr++;
					else
						opt->sa_ipttl = (u_char)tmp;
					break;

				case ADDTTL_FLAG:
					if (!ISu8(tmp))
						typerr++;
					else {
						opt->sa_addttl = (u_char)tmp;
						opt->sa_addttlset = 1;
					}
					break;
				
				case TCPACK_FLAG:
					opt->sa_tcpack = htonl(tmp);
					break;
					
				case TCPSEQ_FLAG:
					opt->sa_tcpseq = htonl(tmp);
					break;
				
				case UDPSPORT_FLAG:
					if (!ISPORT(tmp))
						typerr++;
					else
						opt->sa_udpsport = htons((u_short)tmp);
					break;
					
				case UDPDPORT_FLAG:
					if (!ISPORT(tmp))
						typerr++;
					else
						opt->sa_udpdport = htons((u_short)tmp);
					break;
					
				case TCPSPORT_FLAG:
					if (!ISPORT(tmp))
						typerr++;
					else
						opt->sa_tcpsport = htons((u_short)tmp);
					break;
					
				case TCPDPORT_FLAG:
					if (!ISPORT(tmp))
						typerr++;
					else
						opt->sa_tcpdport = htons((u_short)tmp);
					break;
					
				case TCPWIN_FLAG:
					if (!ISu16(tmp))
						typerr++;
					else
						opt->sa_tcpwin = htons((u_short)tmp);
					break;
					
				case ICMPID_FLAG:
					if (!ISu16(tmp))
						typerr++;
					else
						opt->sa_icmpid = htons((u_short)tmp);
					break;
				
				case ICMPSEQ_FLAG:
					if (!ISu16(tmp))
						typerr++;
					else
						opt->sa_icmpseq = htons((u_short)tmp);
					break;
					
				default:
					typerr++;
					break;
			}

			/* Turn off random flag */
			opt->sa_randflags &= ~field;
		}
	
		/* IP address */
		else if (isip(*(*arr))) {
			tmp = net_inetaddr(*(*arr));
		
			switch(field) {
				
				case BASEADDR_FLAG:
					opt->sa_lip = tmp;
					
					/* Set source address if not set */
					if ((field & SADDR_FLAG) == 0)
						opt->sa_saddr = tmp;										
					break;

				case SADDR_FLAG:
					opt->sa_saddr = tmp;
					opt->sa_saddset = 1;
					break;

				default:
					typerr++;
					break;
			}
		}
		/* SADB file */
		else if (field == SADB_FLAG)
			opt->sa_sadb = strdup(*(*arr));
		else
			typerr++;

		if (typerr) {
			if (field == TCPFLAGS_FLAG)
				fprintf(stderr, "Type error (entry %s): Bad TCP flags\n", ent);
			
			else
				fprintf(stderr, "Type error (entry %s): Bad value '%s' for %s\n",
					ent, *(*arr), *(*arr -2));
		}

		(*arr)++;
		MATCH_OR_DIE(*(*arr), CFG_CMD_END);
		(*arr)++;

		err += typerr;
	}

	MATCH_OR_DIE(*(*arr), CFG_BLOCK_END);
	(*arr)++;

	return(err);
}


/*
 * Chops a string into substrings according to syntax.
 * Arguments:
 * str - String to split up into substrings.
 * Returns a pointer to a NULL terminated array of pointers to
 * u_char on success, and NULL on error with errno set to
 * indicate the error. All pointers returned is allocated using malloc(3),
 * and should be free'd using free(3) when there are no longer in use.
 */
static u_char **
sashcfg_lexer(u_char *str)
{
    u_char **strarr;
    size_t nsize = INIT_BUFSIZE;
    size_t i=0;

    if ( (strarr = malloc(nsize*sizeof(u_char *))) == NULL)
        return(NULL);

    while ((int)*str != (int)NULL) {

        if (i == (nsize -2)) {
            u_char **tmp;

            nsize *= 2;

            if ( (tmp = (u_char **)realloc(strarr,
                    nsize*sizeof(u_char *))) == NULL) {

                if (strarr)
                    free(strarr);

                strarr = NULL;
                return(NULL);
            }
            strarr = tmp;
        }

        /* Skip leading whitespace */
        while (isspace((int)*str))
            str++;

        if ((int)*str == (int)NULL)
            break;

        /* Skip until newline if line-comment token */
        if (*str == CFG_LINE_COMMENT[0]) {
            while (((int)*str != '\n') && ((int)*str != (int)NULL))
                str++;
            continue;
        }

        if (IS_SPEC_CFG_DELIM((int)*str)) {
            strarr[i] = strndup(str, (size_t)1);
            str++;
        }
        else {
            size_t len = 0;
            u_char *pt = str;

            while (((int)*pt != (int)NULL) && (!IS_CFG_DELIM((int)*pt))) {
                pt++;
                len++;
            }
            strarr[i] = strndup(str, len);
            str = pt;
        }
        i++;
    }

    strarr[i] = NULL;
	return(strarr);
}

/*
 * Parse TCP flags.
 * Flags is either in base 2,8,10 or 16,
 * or symbolic names as URG/U, PSH/P ..
 */
static void
parse_tcpflags(u_char ***arr, u_char *typerr, struct sashopt *opt)
{
    long val;

    if (!strisnum(*(*arr), &val)) {

        opt->sa_tcpflags = 0;

        while ((*(*arr) != NULL) && !CMATCH(*(*arr), CFG_CMD_END)) {

            if (CMATCH(*(*arr), "URG") || CMATCH(*(*arr), "U"))
                opt->sa_tcpflags |= 0x20;
            else if (CMATCH(*(*arr), "ACK") || CMATCH(*(*arr), "A"))
                opt->sa_tcpflags |= 0x10;
            else if (CMATCH(*(*arr), "PSH") || CMATCH(*(*arr), "P"))
                opt->sa_tcpflags |= 0x08;
            else if (CMATCH(*(*arr), "RST") || CMATCH(*(*arr), "R"))
                opt->sa_tcpflags |= 0x04;
            else if (CMATCH(*(*arr), "SYN") || CMATCH(*(*arr), "S"))
                opt->sa_tcpflags |= 0x02;
            else if (CMATCH(*(*arr), "FIN") || CMATCH(*(*arr), "F"))
                opt->sa_tcpflags |= 0x01;
            else 
				(*typerr)++;
            (*arr)++;
        }
    }
    else if (!ISu6(val))
		(*typerr)++;
	
	(*arr)--;
}
