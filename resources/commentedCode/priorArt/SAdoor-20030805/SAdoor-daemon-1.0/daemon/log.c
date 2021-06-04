/*
 *  File: log.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor logging routines.
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

#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <netdb.h>
#include "net.h"
#include "sadc.h"
#include "utils.h"
#include "log.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

/* Loccal variables */
static FILE *plog;		
static u_char hostname[MAXHOSTNAMELEN+1];

/* Global variables */
extern struct sa_options opt;

struct log_facility {
	u_char *fac_name;
	u_char fac_val;
} log_facs[] =
{
	{ "auth", LOG_AUTH }, 
/*	{ "authpriv", LOG_AUTHPRIV }, unsupported by SunOS */
	{ "cron", LOG_CRON },
	{ "daemon", LOG_DAEMON },
/*	{ "ftp", LOG_FTP },           unsupported by SunOS */
	{ "kern", LOG_KERN },
	{ "lpr", LOG_LPR },
	{ "mail", LOG_MAIL },
	{ "news", LOG_NEWS },
	{ "syslog", LOG_SYSLOG },
	{ "user" , LOG_USER },
	{ "uucp", LOG_UUCP },
	{ "local0", LOG_LOCAL0 },
	{ "local1", LOG_LOCAL1 },
	{ "local2", LOG_LOCAL2 },
	{ "local3", LOG_LOCAL3 },
	{ "local4", LOG_LOCAL4 },
	{ "local5", LOG_LOCAL5 },
	{ "local6", LOG_LOCAL6 },
	{ "local7", LOG_LOCAL7 },
	{ NULL, -1 }
};

struct log_priority {
	u_char *pri_name;
	u_char pri_val;
} log_prios[] =
{
	{"emerg", LOG_EMERG},
	{"alert", LOG_ALERT},
	{"crit", LOG_CRIT},
	{"err", LOG_ERR},
	{"warning", LOG_WARNING},
	{"notice", LOG_NOTICE},
	{"info", LOG_INFO},
	{"debug", LOG_DEBUG},
	{NULL, -1}
};

/*
 * Translate string into syslog facility
 * returns -1 on error.
 */
int
log_get_facility(u_char *str)
{
	int i;

	for (i = 0; log_facs[i].fac_name != NULL; i++) {
		if (!strcasecmp(str, log_facs[i].fac_name))
			return(log_facs[i].fac_val);
	}
		
	return(-1);
}


/*
 * Translate string into syslog priority
 * returns -1 on error.
 */
int
log_get_priority(u_char *str)
{
	int i;

	for (i = 0; log_prios[i].pri_name != NULL; i++) {
		if (!strcasecmp(str, log_prios[i].pri_name))
			return(log_prios[i].pri_val);
	}

	return(-1);
}

/*
 * Opens private logfile
 * Returns -1 on error, 0 on success.
 */
int
openprivlog(u_char *file, u_char *pname)
{
	u_char *pt;
	
	if (plog != NULL) 
		closeprivlog();
	
	memset(hostname, 0x00, sizeof(hostname));
	gethostname(hostname, sizeof(hostname));

	/* Remove domain */
	if ( (pt = strchr(hostname, '.')) != NULL)
		*pt = '\0';
	
	if ( (plog = fopen(file, "a")) == NULL)	{
		log_sys("Error opening private log file %s: %s\n", 
			opt.sao_privlog, strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * Closes private logfile
 */
 void
 closeprivlog(void)
 {
	fclose(plog);
	plog = NULL;

 }

/*
 * Writes messages to private log if verbose level is greater than 0
 */
void
log_priv(const u_char *fmt, ...)
{
	va_list ap;
	
	if ((plog != NULL) && (opt.sao_privbose > 0)) {
		fprintf(plog, "%s %s %s[%u]: ", timestr(), hostname, 
			opt.sao_pname, (u_int)getpid());
		va_start(ap, fmt);
		vfprintf(plog, fmt, ap);
		va_end(ap);
		fflush(plog);
	}
}

/*
 * Writes messages to syslog if verbose level is greater than 0
 */
void
log_sys(const u_char *fmt, ...)
{
	va_list ap;

	if (opt.sao_sysvbose > 0) {
		va_start(ap, fmt);
		vsyslog(opt.sao_logpri, fmt, ap);
		va_end(ap);
	}
}
