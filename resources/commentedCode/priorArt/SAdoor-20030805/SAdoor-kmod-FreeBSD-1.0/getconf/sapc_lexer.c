/*
 *  File: sapc_lexer.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor packet config lexer routines.
 *  Version: 1.0
 *  Date: Mon Jan  6 00:13:13 CET 2003
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "sapc.h"
#include "utils.h"

/* Number of pointers to allocate at start */
#define INIT_BUFSIZE    512

/* 
 * Chops a string into substrings according to syntax. 
 * Arguments:
 * str - String to split up into substrings.
 * Returns a pointer to a NULL terminated array of pointers to 
 * u_char on success, and NULL on error with errno set to 
 * indicate the error. All pointers returned is allocated using malloc(3),
 * and should be free'd using free(3) when there are no longer in use.
 */
u_char **
sapc_lexer(u_char *str)
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
        if (*str == LINE_COMMENT[0]) {
            while (((int)*str != '\n') && ((int)*str != (int)NULL))
                str++;
            continue;
        }

        if (IS_SPEC_DELIM((int)*str)) {
            strarr[i] = strndup(str, (size_t)1);
            str++;
        }
        else {
            size_t len = 0;
            u_char *pt = str;
            
            while (((int)*pt != (int)NULL) && (!IS_DELIM((int)*pt))) {
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
