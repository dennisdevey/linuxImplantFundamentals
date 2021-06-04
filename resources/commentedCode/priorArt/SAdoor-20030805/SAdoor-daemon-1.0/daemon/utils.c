/*
 *  File: utils.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Common used routines.
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
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include "utils.h"

/*
 * Allocates sufficient memory for a copy of the first
 * n characters in str, does the copy and returns a pointer to it.
 * If str is shorter than n, all available characters is copyed.
 * The memory is allocated using malloc(3), so it should be free'd
 * using free(3) when it is no longer in use.
 * On error NULL is returned and the global variable errno is set
 * to indicate the error.
 */
u_char *
strndup(u_char *str, const size_t n)
{
    u_char *newstr;

    if ( (newstr = malloc(n*sizeof(u_char)+1)) == NULL)
        return(NULL);

    strncpy(newstr, str, n);
    newstr[n] = '\0';
	return(newstr);
}

/*
 * Returns 1 if str is a dotted decimal IP address,
 * 0 otherwise.
 */
int
isip(u_char *str)
{
    int ip[4];
    u_char c;

    if (sscanf(str, "%d.%d.%d.%d%c", &ip[0], &ip[1], &ip[2], &ip[3], &c) != 4)
        return(0);

    return(ISu8(ip[0]) && ISu8(ip[1]) && ISu8(ip[2]) && ISu8(ip[3]));
}

/*
 * Check is string is a numeric value.
 * Zero is returned if string contains base-illegal
 * characters, 1 otherwise.
 * Binary value should have the prefix '0b'
 * Octal value should have the prefix '0'.
 * Hexadecimal value should have the prefix '0x'.
 * A string with any digit as prefix except '0' is
 * interpreted as decimal.
 * If val in not NULL, the converted value is stored
 * at that address.
 */
int
strisnum(u_char *str, long *val)
{
    int base = 0;
    char *endpt;

    if (str == NULL)
        return(0);

    while (isspace((int)*str))
        str++;

    /* Binary value */
    if (!strncmp(str, "0b", 2) || !strncmp(str, "0B", 2)) {
        str += 2;
        base = 2;
    }

    if (*str == '\0')
        return(0);

    if (val == NULL)
        strtol(str, &endpt, base);
    else
        *val = strtol(str, &endpt, base);

    return((int)*endpt == '\0');
}

/*
 * Returns current time
 */
u_char *
timestr(void)
{
	static u_char tstr[256];
	time_t caltime;
	struct tm *tm;
	
	memset(tstr, 0x00, sizeof(tstr));
	
	if ( (caltime = time(NULL)) == -1) {
		snprintf(tstr, sizeof(tstr) -1, "caltime(): %s", 
				strerror(errno));
	}

	else if ( (tm = localtime(&caltime)) == NULL) {
		snprintf(tstr, sizeof(tstr) -1, "localtime(): %s", 
				strerror(errno));
	}

	else if (strftime(tstr, sizeof(tstr) -1, "%b %d %T", tm) == 0) {
		snprintf(tstr, sizeof(tstr) -1, "localtime(): %s", 
			strerror(errno));
	}
	
	return(tstr);
}

/*
 * Appends filename to directory path.
 */ 
u_char *
mkfpath(u_char *dir, u_char *file)
{
	static u_char abspath[MAXPATHLEN+1];
	
	memset(abspath, 0x00, sizeof(abspath));
	snprintf(abspath, sizeof(abspath), "%s/%s", dir, file);
	return(abspath);
}


/*
 * Write N bytes to a file descriptor
 */
ssize_t
writen(int fd, void *buf, size_t n)
{
	size_t tot = 0;
	ssize_t w;

	do {
		if ( (w = write(fd, (void *)((u_char *)buf + tot), n - tot)) <= 0)
			return(w);
		tot += w;
	} while (tot < n);
	
	return(tot);
}

/*
 * Read N bytes from a file descriptor
 */
ssize_t
readn(int fd, void *buf, size_t n)
{
	size_t tot = 0;
	ssize_t r;
	
	do {
		if ( (r = read(fd, (void *)((u_char *)buf + tot), n - tot)) <= 0)
			return(r);
		tot += r;
	} while (tot < n);
	
	return(tot);	
}

/*
 * Locate bytes in buffer.
 * Returns index to start of str in buf on success, 
 * -1 if string was not found.
 */
ssize_t
memstr(void *big, size_t blen, const void *little, size_t llen)
{
    ssize_t i = 0;

    if (blen < llen)
        return(-1);

    while ( (blen - i) >= llen) {
        if (memcmp((u_char *)big + i, little, llen) == 0)
            return(i);
        i++;
    }

    return(-1);
}

/*
 * Returns 1 if file descriptor is ready for writing,
 * 0 otherwise, and -1 on error.
 */
int
ready_write(int fd)
{
    fd_set writeset;
    struct timeval tv;

    memset(&tv, 0x00, sizeof(tv));
    FD_ZERO(&writeset);
    FD_SET(fd, &writeset);

    if (select(fd+1, NULL, &writeset, NULL, &tv) > 0)
        return(1);

    return(0);
}

