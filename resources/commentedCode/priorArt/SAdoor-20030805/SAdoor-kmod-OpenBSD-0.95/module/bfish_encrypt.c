/*
 *  File: bfish_encrypt.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Blowfish encryption routine
 *  Version: 1.0
 *  Date: Thu Oct 17 19:40:50 CEST 2002
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

#include <sys/types.h>
#include "bfish.h"

#define SWAP(a,b)   (a)^=(b); (b)^=(a); (a)^=(b)

/*
 * Argumets:
 * left    - Address to left side 32 bit block to encrypt
 * right   - Address to right side 32 bit block to encrypt
 * bk      - The blowfish key initialized by bfish_keyinit()
 * swap    - Converts encrypted blocks to big endian if this is true
 */
void
bfish_encrypt_swap(uint32_t *left, uint32_t *right, struct bfish_key *bk, int swap)
{
    register uint32_t i;
    register uint32_t fxl;


    /* Convert to big endian */
    if (swap) {
        int little = 1;

        if ((*(char *)&little)) {
            *left = (((*left >> 24) | (*left << 24)) |
                (((*left >> 8) & 0xff00) | ((*left << 8) & 0xff0000)));

            *right = (((*right >> 24) | (*right << 24)) |
                (((*right >> 8) & 0xff00) | ((*right << 8) & 0xff0000)));
        }
    } 

    for (i=0; i<16; i++) {
        
        *left ^= bk->bk_pbox[i];

        /* The "F" function */
        
        /* fxl = s1[a] + s2[b] */
        fxl = bk->bk_sbox[0][(*left >> 24) & 0xff] + 
            bk->bk_sbox[1][(*left >> 16) & 0xff];

        /* fxl = fxl XOR s3[c] */
        fxl ^= bk->bk_sbox[2][(*left >> 8) & 0xff];

        /* fxl = fxl + s4[d] */
        fxl += bk->bk_sbox[3][*left & 0xff];
    
        *right ^= fxl;

        SWAP(*left, *right);
    }

    /* Swap back last swap */
    SWAP(*left, *right);
    
    *left ^= bk->bk_pbox[17];
    *right ^= bk->bk_pbox[16];

    /* Convert to big endian */
    if (swap) {   
        int little = 1;
        
        if ((*(char *)&little)) {
            
            *left = (((*left >> 24) | (*left << 24)) | 
                (((*left >> 8) & 0xff00) | ((*left << 8) & 0xff0000)));
            
            *right = (((*right >> 24) | (*right << 24)) |  
                (((*right >> 8) & 0xff00) | ((*right << 8) & 0xff0000)));
        }
    } 
}
