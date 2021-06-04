/*
 *  File: sadc.h
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor configuration parser/typecheck header file.
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

#ifndef _SADC_H
#define _SADC_H

#include <sys/types.h>

/* Timeout in seconds */
#define CONNECTION_TIMEOUT		30

/* Options */
struct sa_options {
	u_char *sao_pname;		/* argv[0] */
	u_char *sao_pktconf;	/* Packet config file */
	u_char *sao_saconf;		/* Daemon config file */
	u_char *sao_keyfile;	/* Blowfish keyfile */
	u_char *sao_pidfile;    /* PID file */
	u_char *sao_privlog;	/* Private logfile */
	u_char *sao_sadbfile;	/* SAdoor database file */

	u_int sao_ipv4addr;		/* IPv4 address of host */
	u_char sao_bfkey[56];   /* Blowfish key */
	u_char sao_sysvbose;	/* Syslog Verbose level */
	u_char sao_privbose;    /* Private log verbose level */
	u_char *sao_iface;		/* Network Interface */
	u_char sao_logfac;		/* Syslog facility */
	u_char sao_logpri;		/* Syslog level/priority */
	u_char sao_promisc:1;	/* Promiscuous flag */
	u_char sao_usepidf:1;	/* Use PID file flag */
	u_char sao_nosingle:1;	/* Disable single command */
	u_char sao_noaccept:1;  /* Disable accept command */
	u_char sao_noconnect:1; /* Disable connect command */
	u_char sao_noftrans:1;  /* Disable file transfer */
	u_char sao_protrep:1;   /* Replay protection enabled */
	u_char sao_resolve:1;	/* Resolve hostnames when logging */	
	time_t sao_ptimeout;	/* Packets timeout */
	long sao_ctout;			/* Connection timeout */
	u_char *sao_shell;		/* Program to run upon connect */
	u_char *sao_defcmd;		/* Default command */
};

#define IS_SACONF_SPACE(c)	(((c) == ' ') || ((c) == '\t'))

/* Maximum length of a line in config file */
#define SACONF_MAXLINE		4095

/* Config file tokens */
#define SACONF_LINECOMMENT		'#'
#define SACONF_BOOL_TRUE		"yes"
#define SACONF_BOOL_FALSE		"no"
#define SACONF_INETADDR			"IPv4Address"
#define SACONF_SYSVERBOSE		"SyslogVerboseLevel"
#define SACONF_PRIVVERBOSE		"PrivateLogVerboseLevel"
#define SACONF_PRIVLOGFILE		"PrivateLogFile"
#define SACONF_KEYFILE			"PrivateKeyFile"
#define SACONF_PACKETFILE		"PacketConfigFile"
#define SACONF_SADBFILE			"SADBFile"
#define SACONF_IFACE			"ListenIface"
#define SACONF_SYSLOG_FACILITY	"SyslogFacility"
#define SACONF_SYSLOG_PRIORITY	"SyslogPriority"
#define SACONF_RUNPROMISC		"RunPromisc"
#define SACONF_PKTS_TIMEOUTSEC	"PacketsTimeoutSec"
#define SACONF_SHELL			"RunOnConnect"
#define SACONF_PIDFILE			"PIDfile"
#define SACONF_USEPIDFILE		"CreatePIDFile"
#define SACONF_NOSINGLECMD		"DisableRunCommand"
#define SACONF_NOACCEPTCMD      "DisableAcceptCommand"
#define SACONF_NOCONNECT_CMD	"DisableConnectCommand"
#define SACONF_NOFILETRANSFER   "DisableFiletransfer"
#define SACONF_ENABLE_REPPROT   "EnableReplayProtection"
#define SACONF_DEFAULT_CMD		"NULLCommand"

/* Error message to send when file transfer is disabled */
#define FTRANS_DISABLED_ERRORMSG	"File transfer disabled!"

/* sadc.c */
extern int sadc_readconf(u_char *, struct sa_options *);

#endif /* _SADC_H */
