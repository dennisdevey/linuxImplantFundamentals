/*
 *  File: sadc.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor configuration parser/typecheck routines.
 *  Version: 1.0
 *  Date: Tue Jan  7 23:24:15 CET 2003
 *
 *  Copyright (c) 2002 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
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
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "net.h"
#include "utils.h"
#include "log.h"
#include "sadc.h"

/*
 * Read config file and set values in the
 * sa_options structure pointed to by opt.
 * Returns -1 on error, 0 on success.
 */
int
sadc_readconf(u_char *file, struct sa_options *opt)
{
	u_char line[SACONF_MAXLINE+1];
	size_t lcount = 0;
	u_char *arg0;
	u_char *pt;
	long val;
	size_t typerr;
	size_t err = 0;
	FILE *cf;

	if ( (cf = fopen(file, "r")) == NULL) {
		fprintf(stderr, "sadc_readconf(): open(%s): %s\n",
			file, strerror(errno));
		return(-1);
	}

	memset(line, 0x00, sizeof(line));

	while ( (arg0 = fgets(line, SACONF_MAXLINE, cf)) != NULL) {
		u_char *arg1;
		u_char *arg2;
		
		lcount++;
		typerr = 0;

		/* Remove any leading spaces */
		while (IS_SACONF_SPACE((int)*arg0))
			arg0++;

		/* Remove newline and comment */
		for (pt = arg0; (int)*pt != '\0'; pt++) {

			if ((int)*pt == SACONF_LINECOMMENT) 
				*pt = '\0';

			if ((int)*pt == '\n') {
				*pt = '\0';
				break;
			}
		}

		if ((int)*arg0 == '\0')
			continue;

		/* NULL terminate arg0 and arg1, and let arg2 point to rest of line */
		arg1 = arg0;
		while ((*arg1 != '\0') && !IS_SACONF_SPACE((int)*arg1))
			arg1++;
		
		*arg1++ = '\0';

		while (IS_SACONF_SPACE((int)*arg1))
			arg1++;

		arg2 = arg1;
		while ((*arg2 != '\0') && !IS_SACONF_SPACE((int)*arg2))
			arg2++;

		*arg2++ = '\0';

		while (IS_SACONF_SPACE((int)*arg2))
			arg2++;

		
		if (!strncasecmp(arg0, SACONF_SYSVERBOSE, 
				sizeof(SACONF_SYSVERBOSE)-1)) {
		
			if (!strisnum(arg1, &val)) 
				typerr++;
			else if (!ISu8(val))
				typerr++;
			else
				opt->sao_sysvbose = (u_char)val;
		}
	
		else if (!strncasecmp(arg0, SACONF_INETADDR,
				sizeof(SACONF_INETADDR)-1)) {
			
			if (isip(arg1) == 0)
				typerr++;
			else
				opt->sao_ipv4addr = net_inetaddr(arg1);
		}
		
		else if (!strncasecmp(arg0, SACONF_PRIVVERBOSE, 
				sizeof(SACONF_PRIVVERBOSE)-1)) {
			
			if (!strisnum(arg1, &val))
				typerr++;
			else if (!ISu8(val))
				typerr++;
			else
				opt->sao_privbose = (u_char)val;
		}
		
		else if (!strncasecmp(arg0, SACONF_IFACE, 
				sizeof(SACONF_IFACE)-1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else
				opt->sao_iface = strdup(arg1);
		}

		else if (!strncasecmp(arg0, SACONF_SYSLOG_FACILITY, 
				sizeof(SACONF_SYSLOG_FACILITY)-1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else if ( (val = log_get_facility(arg1)) == -1)
				typerr++;
			else 
				opt->sao_logfac = (u_char)val;
		}

		else if (!strncasecmp(arg0, SACONF_SYSLOG_PRIORITY, 
				sizeof(SACONF_SYSLOG_PRIORITY) -1)) {
		
			if (*arg1 == '\0')
				typerr++;
			else if ( (val = log_get_priority(arg1)) == -1)
				typerr++;
			else
				opt->sao_logpri = (u_char)val;
		}

		else if (!strncasecmp(arg0, SACONF_RUNPROMISC, 
				sizeof(SACONF_RUNPROMISC) -1)) {
			
			if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
				opt->sao_promisc = 1;
		
			else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
				opt->sao_promisc = 0;
			else
				typerr++;
		}

		else if (!strncasecmp(arg0, SACONF_NOSINGLECMD,
			sizeof(SACONF_NOSINGLECMD) -1)) {
			
			if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
				opt->sao_nosingle = 1;
				
			else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
				opt->sao_nosingle = 0;
			else
				typerr++;
		}

        else if (!strncasecmp(arg0, SACONF_NOACCEPTCMD,
            sizeof(SACONF_NOACCEPTCMD) -1)) {
            
            if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
                opt->sao_noaccept = 1;
                
            else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
                opt->sao_noaccept = 0;
            else
                typerr++;
        }

		else if (!strncasecmp(arg0, SACONF_NOCONNECT_CMD,
			sizeof(SACONF_NOCONNECT_CMD) -1)) {

			if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
				opt->sao_noconnect = 1;

			else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
				opt->sao_noconnect = 0;
			else
				typerr++;
		}

        else if (!strncasecmp(arg0, SACONF_NOFILETRANSFER,
            sizeof(SACONF_NOFILETRANSFER) -1)) {

            if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
                opt->sao_noftrans = 1;

            else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
                opt->sao_noftrans = 0;
            else
                typerr++;
        }

        else if (!strncasecmp(arg0, SACONF_ENABLE_REPPROT,
            sizeof(SACONF_ENABLE_REPPROT) -1)) {

            if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
                opt->sao_protrep = 1;

            else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
                opt->sao_protrep = 0;
            else
                typerr++;
        }

		else if (!strncasecmp(arg0, SACONF_PKTS_TIMEOUTSEC, 
				sizeof(SACONF_PKTS_TIMEOUTSEC) -1)) {
			
			if (!strisnum(arg1, &val))
				typerr++;
			else
				opt->sao_ptimeout = (time_t)val;
		}

		else if (!strncasecmp(arg0, SACONF_SHELL, 
				sizeof(SACONF_SHELL) -1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else 
				opt->sao_shell = strdup(arg1);
		}	
		
		else if (!strncasecmp(arg0, SACONF_PIDFILE, 
				sizeof(SACONF_PIDFILE) -1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else if (*arg1 != '/')
				typerr++;
			else
				opt->sao_pidfile = strdup(arg1);
		}

        else if (!strncasecmp(arg0, SACONF_USEPIDFILE,
                sizeof(SACONF_USEPIDFILE) -1)) {

            if (!strcasecmp(arg1, SACONF_BOOL_TRUE))
                opt->sao_usepidf = 1;
            else if (!strcasecmp(arg1, SACONF_BOOL_FALSE))
                opt->sao_usepidf = 0;
            else
                typerr++;
        }

		else if (!strncasecmp(arg0, SACONF_PRIVLOGFILE, 
				sizeof(SACONF_PRIVLOGFILE) -1)) {
		
			if (*arg1 == '\0')
				typerr++;
			else if (*arg1 != '/')
				typerr++;
			else
				opt->sao_privlog = strdup(arg1);

		}
		
		else if  (!strncasecmp(arg0, SACONF_PACKETFILE,
				sizeof(SACONF_PACKETFILE) -1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else if (*arg1 != '/')
				typerr++;
			else
				opt->sao_pktconf = strdup(arg1);

		}

		else if  (!strncasecmp(arg0, SACONF_KEYFILE,
				sizeof(SACONF_KEYFILE) -1)) {

			if (*arg1 == '\0')
				typerr++;
			else if (*arg1 != '/')
				typerr++;
			else
				opt->sao_keyfile = strdup(arg1);
		}

		else if  (!strncasecmp(arg0, SACONF_SADBFILE,
				sizeof(SACONF_SADBFILE) -1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else if (*arg1 != '/')
				typerr++;
			else
				opt->sao_sadbfile = strdup(arg1);
		}
		
		else if (!strncasecmp(arg0, SACONF_DEFAULT_CMD,
				sizeof(SACONF_DEFAULT_CMD) -1)) {
			
			if (*arg1 == '\0')
				typerr++;
			else {
				u_char cbuf[SACONF_MAXLINE+1];
				memset(cbuf, 0x00, sizeof(cbuf));
				
				snprintf(cbuf, sizeof(cbuf)-1, "%s %s", arg1, arg2);
				opt->sao_defcmd = strdup(cbuf);
			}
		}

		else {
			u_char *tmp = arg0;
			
			while ((*tmp != '\0') && !IS_SACONF_SPACE((int)*tmp))
				tmp++;
			*tmp = '\0';
			
			fprintf(stderr, "Syntax Error: Bad configuration option: '%s' on line %u in %s\n",
				arg0, lcount, file);
			err++;
		}
	
		if (typerr != 0) {
			
			if (*arg1 == '\0')
				fprintf(stderr, "Syntax Error on line %u: Missing value for %s\n",
					lcount, arg0);
			else
				fprintf(stderr, "Type Error on line %u: Bad value for %s: '%s'\n",
					lcount, arg0, arg1);
			err++;
		}

		memset(line, 0x00, sizeof(line));
	}

	if (ferror(cf)) {
		fprintf(stderr, "sadc_readconf(): Error reading input file\n");
		err++;
	}

	fclose(cf);
	return(err ? -1 : 0);
}
