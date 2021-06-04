/*
 *  File: random.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Random number routines.
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include "random.h"
#include "utils.h"

/*
 * Fill buf with buflen number of random bytes.
 */
u_char *
random_bytes(u_char *buf, size_t buflen)
{
	int fd = -1;

	if ( (fd = open("/dev/srandom", O_RDONLY)) < 0) {
		if ( (fd = open("/dev/arandom", O_RDONLY)) < 0) {   
			if ( (fd = open("/dev/random", O_RDONLY)) < 0) {
				if ( (fd = open("/dev/prandom", O_RDONLY)) < 0) {
					fd = open("/dev/urandom", O_RDONLY);
				}
			}
		}
	}

	/* No random device found */
	if (fd < 0) {
		random_bytes_weak(buf, buflen);
	}
	else {
		if (readn(fd, buf, buflen) != buflen) {	
			close(fd);
			return(NULL);
		}
		close(fd);
	}

	return(buf);
}

/*
 * Generate "weak" random bytes.
 */
void
random_bytes_weak(u_char *buf, size_t buflen)
{
	while (buflen > 0) {
		*(buf++) = (u_char)random_int(0, 1 << (8*sizeof(u_char)-1) );
		buflen--;
		usleep(1);
	}
}

/*
 * Generates a random integer (low <= x <= high)
 */
int
random_int(int low, int high)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srand((tv.tv_sec ^ tv.tv_usec) ^ getpid());
    return(rand() % (high - low  + 1) + low);
}

/*
 * Randomize a network byte ordered IP address.
 * We skip 0, 10, 44, 127, 172, 192, and bigger than 223.
 */
long
random_ip(void)
{
    long addr;

    do { 

        addr = random_int(1, 223);

    } while ((addr == 0) || (addr == 10) || (addr == 44) ||
            (addr == 127) || (addr == 172) || (addr == 192));

    addr <<= 24;
    addr |= (random_int(1, 254) << 16);
    addr |= (random_int(1, 254) << 8);
    addr |= random_int(1, 254);
    return(htonl(addr));
}

