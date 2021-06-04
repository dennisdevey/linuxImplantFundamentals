/*
 *  File: utils.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Common used routines.
 *  Version: 1.0
 *  Date: 
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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include "utils.h"

/* 4.3+BSD require this to map files */
#ifndef MAP_FILE
#define MAP_FILE 0	/* For other systems */
#endif

/* Default mapping mode */
#define MAP_PROT	(PROT_READ | PROT_WRITE)
#define MAP_FLAGS	(MAP_FILE | MAP_PRIVATE)

#ifndef ISu8
#define ISu8(x) (((x) >= 0) && ((x) <= 0xff))
#endif

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
 * Memory map file read and write in private mode.
 * On success a pointer to a mfile structure is returned,
 * NULL on error.
 */
struct mfile *
open_mfile(u_char *file)
{
	int fd;
	struct mfile *mf;
	caddr_t mmad;
	struct stat sb;	/* For file size */

	/* Open file */
	if( (fd = open(file, O_RDONLY)) < 0) {
		fprintf(stderr, "** Error: open(): %s: %s\n", file, 
			strerror(errno));
		return(NULL);
	}
	
	/* Get size of file */
	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "** Error: mmapfile(): fstat(): %s: %s\n", file, 
			strerror(errno));
		return(NULL);
	}

	/* Memory map file */
	if ( (mmad = mmap(0, sb.st_size, MAP_PROT, MAP_FLAGS, fd, 0)) == MAP_FAILED) {
		fprintf(stderr, "** Error: mmap(): %s", strerror(errno));
		close(fd);
		return(NULL);
	}
		
	if ( (mf = (struct mfile *)malloc(sizeof(struct mfile))) == NULL) {
		fprintf(stderr, "** Error: malloc(): %s", strerror(errno));
		return(NULL);
	}
	mf->mf_fd = fd;
	mf->mf_size = sb.st_size;
	mf->mf_file = (u_char *)mmad;
	return(mf);
}


/*
 * Close memory mapped file and free allocated memory.
 * Returns -1 on error and zero on success.
 */
int
close_mfile(struct mfile *mf)
{
	if (mf == NULL) {
		fprintf(stderr, "close_mfile(): Received NULL pointer as mfile\n");
		return(-1);
	}

	if (munmap((void *)mf->mf_file, mf->mf_size) < 0) {
		perror("close_mfile(): munmap()");
		return(-1);
	}
	
	if (close(mf->mf_fd) < 0) {
		perror("close_mfile(): close()");
		return(-1);
	}

	free(mf);
	return(0);
}

/*
 * Returns current time as a string.
 * If sec is not zero, sec is translated.
 * If fmt is not NULL fmt is used as format string 
 * to strftime.
 */
#define DEFAULT_TIMEFMT		"%T %b %d %Y"
u_char *
timestr(time_t sec, u_char *fmt)
{
    static u_char tstr[256];
    time_t caltime;
    struct tm *tm;

	if (fmt == NULL)
		fmt = DEFAULT_TIMEFMT;

	if (sec == 0)
		sec = time(NULL);

    memset(tstr, 0x00, sizeof(tstr));

    if ( (caltime = sec) == -1) {
        snprintf(tstr, sizeof(tstr) -1, "caltime(): %s",
                strerror(errno));
    }

    else if ( (tm = localtime(&caltime)) == NULL) {
        snprintf(tstr, sizeof(tstr) -1, "localtime(): %s",
                strerror(errno));
    }

    else if (strftime(tstr, sizeof(tstr) -1, fmt, tm) == 0) {
        snprintf(tstr, sizeof(tstr) -1, "localtime(): %s",
            strerror(errno));
    }

    return(tstr);
}

/*
 * Write N bytes to a file descriptor
 */
ssize_t
writen(int fd, const void *buf, size_t n)
{
    size_t tot = 0;
    ssize_t w;

    do {
        if ( (w = write(fd, (void *)(buf + tot), n - tot)) <= 0)
            return(w);
        tot += w;
    } while (tot < n);

    return(tot);
}

/*
 * Read N bytes from a file descriptor
 */
ssize_t
readn(int fd, const void *buf, size_t n)
{
    size_t tot = 0;
    ssize_t r;

    do {
        if ( (r = read(fd, (void *)(buf + tot), n - tot)) <= 0)
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
memstr(const void *big, size_t blen, const void *little, size_t llen)
{
    ssize_t i = 0;

    if (blen < llen)
        return(-1);

    while ( (blen - i) >= llen) {
        if (memcmp((big + i), little, llen) == 0)
            return(i);
        i++;
    }

    return(-1);
}


/*
 * Returns 1 if file descriptor is ready for writing, 0 otherwise.
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
 * Check if string is a numeric value.
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
 * Get encryption key.
 * If type is NEW_KEY, the key has to be entered twice.
 * If type is FILE_KEY, we simply return the key.
 * Returns a pointer to the entered key on succes, and NULL
 * pointer on error.
 */
u_char *
getkey(u_char type, const u_char *file)
{
	u_char prompt[512];
    char *key = NULL;

	if (file == NULL)
		snprintf(prompt, sizeof(prompt), "Enter passphrase: ");
	else
		snprintf(prompt, sizeof(prompt), "Passphrase for '%s': ", file);

#ifdef HAVE_GETPASSPHRASE
    key = getpassphrase(prompt);
#else
    key = getpass(prompt);
#endif

    while( (strlen(key) < 6) || (strlen(key) > 56) ) {
        fprintf(stderr, "Key error, minimum 6, maximum 56 characters!\n");
#ifdef HAVE_GETPASSPHRASE
        key = getpassphrase(prompt);
#else
        key = getpass(prompt);
#endif
    }

    if(type == NEW_KEY) {
        char *key1 = strdup(key);
        key = getpass("Retype passphrase: ");

        if(strcmp(key, key1)) {
            fprintf(stderr, "Passphrases mismatch!\n");
            free(key1);
            return(NULL);
        }
        free(key1);
    }
    return(key);
}

