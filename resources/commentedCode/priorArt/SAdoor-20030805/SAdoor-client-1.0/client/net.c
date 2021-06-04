/*
 *  File: net.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Common used network routines.
 *  Version: 1.0
 *  Date:
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


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "net.h"

/*
 * Translate hostname or dotted decimal host address
 * into a network byte ordered IP address.
 * Returns -1 on error.
 */
long
net_inetaddr(u_char *host)
{
    long haddr;
    struct hostent *hent;

    if ( (haddr = inet_addr(host)) < 0) {
        if ( (hent = gethostbyname(host)) == NULL)
            return(-1);

        memcpy(&haddr, (hent->h_addr), 4);
    }
    return(haddr);
}

/*
 * Translate network byte ordered IP address into its 
 * dotted decimal representation.
 * The translated address is written to the location of ipa. 
 * If ipa is NULL a static pointer is returned.
 */
u_char *
net_ntoa(u_long ip, u_char *ipaa)
{
	static u_char ipas[16];
	struct in_addr ipa;
	
	ipa.s_addr = ip;

	if (ipaa != NULL) {
		sprintf((char *)ipaa, "%s", inet_ntoa(ipa));
		return(ipaa);
	}
	else
		sprintf((char *)ipas, "%s", inet_ntoa(ipa));
	
	return(ipas);
}

/*
 * Returns official name of host from network byte 
 * ordered IP address on success, NULL on error.
 */
u_char *
net_hostname(struct in_addr *addr)
{
	static u_char hname[MAXHOSTNAMELEN+1];
    struct hostent *hent;

    if ( (hent = gethostbyaddr((char *)&(addr->s_addr),
            sizeof(addr->s_addr), AF_INET)) == NULL) {
        return(NULL);
    }
	snprintf(hname, sizeof(hname) -1, "%s", hent->h_name);
    return(hname);
}

/*
 * Returns official name of host from network byte 
 * ordered IP address on success, NULL on error.
 */
u_char *
net_hostname2(u_int ip)
{
	struct in_addr ipa;
	ipa.s_addr = ip;
	return(net_hostname(&ipa));
}

/*
 * Return network byte ordered
 * IP address of local host.
 */
long
net_localip(void)
{
    u_char lname[MAXHOSTNAMELEN +1];
    struct in_addr addr;
    memset(lname, 0x00, sizeof(lname));

    if ( gethostname(lname, MAXHOSTNAMELEN) < 0)
        return(-1);

    addr.s_addr = net_inetaddr(lname);
    return(addr.s_addr);
}

/*
 * Convert TCP control flags into string.
 */
u_char *
net_tcpflags(u_short flags)
{
    static u_char buf[28];
	memset(buf, 0x00, sizeof(buf));

    /* FIN No more data from sender */
    if (flags & 0x01)
        strncat(buf, "FIN ", 4);

    /* SYN:  Synchronize sequence numbers */
    if (flags & 0x02)
        strncat(buf, "SYN ", 4);

    /* RST:  Reset the connection */
    if (flags & 0x04)
        strncat(buf, "RST ", 4);

    /* PSH:  Push Function */
    if (flags & 0x08)
        strncat(buf, "PSH ", 4);

    /* ACK:  Acknowledgment field significant */
    if (flags & 0x010)
        strncat(buf, "ACK ", 4);

    /* URG:  Urgent Pointer field significant */
    if (flags & 0x020)
        strncat(buf, "URG ", 4);

    return(buf);
}

/*
 * Convert TCP control flags into string.
 */
u_char *
net_tcpflags_short(u_short flags)
{
    static u_char buf[8];
	u_char *pt = buf;
    memset(buf, 0x00, sizeof(buf));

    /* FIN No more data from sender */
    if (flags & 0x01)
        *pt++ = 'F';

    /* SYN:  Synchronize sequence numbers */
    if (flags & 0x02)
        *pt++ = 'S';

    /* RST:  Reset the connection */
    if (flags & 0x04)
        *pt++ = 'R';

    /* PSH:  Push Function */
    if (flags & 0x08)
        *pt++ = 'P';

    /* ACK:  Acknowledgment field significant */
    if (flags & 0x010)
        *pt++ = 'A';

    /* URG:  Urgent Pointer field significant */
    if (flags & 0x020)
        *pt++ = 'U';

    return(buf);
}

