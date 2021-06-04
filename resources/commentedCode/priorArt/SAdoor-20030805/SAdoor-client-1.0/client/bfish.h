/*
 *  File: bfish.h
 *  Description: Main header file for libbfish
 *  Version: 1.0
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Date: Tue Oct 22 06:30:44 CEST 2002
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
  
#ifndef _BFISH_H
#define _BFISH_H

#include <sys/types.h>
#include <netinet/in.h> /* Linux needs this for uint8_t & CO */
#include <string.h>     /* Linux needs this for mem*() */

#define SWAP_LIL_ENDIAN    1
#define NO_ENDIAN_SWAP     0

/*
 * The key structure
 */
struct bfish_key {
    uint32_t bk_pbox[18];    
    uint32_t bk_sbox[4][256];
};

/* bfish_keyinit.c */
extern struct bfish_key *bfish_keyinit(uint8_t *, uint16_t);

/* bfish_encrypt.c */
extern void bfish_encrypt_swap(uint32_t *, uint32_t *, struct bfish_key *,int);
#define bfish_encrypt(left,right,bk) \
    bfish_encrypt_swap((left), (right), (bk), SWAP_LIL_ENDIAN)

/* bfish_decrypt.c */
extern void bfish_decrypt(uint32_t *, uint32_t *, struct bfish_key *);

/* bfish_ofb.c */
extern int bfish_ofb8_setiv(uint8_t *, uint8_t, uint8_t *, uint8_t *);
extern void bfish_ofb8_cleariv(void);
extern void bfish_ofb8_encrypt(uint8_t *, uint16_t);
extern void bfish_ofb8_decrypt(uint8_t *, uint16_t);

/* bfish_cfb.c */
extern void bfish_cfb(uint8_t *, uint32_t, uint8_t *, uint8_t, struct bfish_key *, uint8_t);
#define bfish_cfb_encrypt(str, slen, iv, bsize, bk) bfish_cfb(str, slen, iv, bsize, bk, 1)
#define bfish_cfb_decrypt(str, slen, iv, bsize, bk) bfish_cfb(str, slen, iv, bsize, bk, 0)

/* bfish_cbc_encrypt.c */
extern void bfish_cbc_encrypt(uint8_t *, uint32_t, uint8_t *, struct bfish_key *);

/* bfish_cbc_decrypt.c */
extern void bfish_cbc_decrypt(uint8_t *, uint32_t, uint8_t *, struct bfish_key *);


#endif /* _BFISH_H */
