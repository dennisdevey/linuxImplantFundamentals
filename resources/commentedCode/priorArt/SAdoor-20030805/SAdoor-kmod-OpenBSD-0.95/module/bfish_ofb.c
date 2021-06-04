/*
 *  File: bfish_ofb.c
 *  Description: Blowfish output feedback mode.
 *  Version: 1.0
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Date: Mon Feb 17 12:00:22 CET 2003
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
#include <sys/param.h>
#include <sys/malloc.h>
#include "sadoor.h"

/*
 * Initiate Output Feedback Mode.
 * bfkey - The blowfish key
 * klen  - Length of bfkey in bytes
 * eiv - The 8 byte long IV to be used for encryption.
 * div - The 8 byte long IV to be used for decryption.
 */
struct bfish_ofb *
bfish_ofb8_setiv(uint8_t *bfkey, uint8_t klen, uint8_t *eiv, uint8_t *div)
{
    struct bfish_ofb *ob;

    ob = (struct bfish_ofb *)kspace_calloc(sizeof(struct bfish_ofb));
    
    memcpy(ob->enciv, eiv, sizeof(ob->enciv));
    memcpy(ob->deciv, div, sizeof(ob->deciv));
    ob->eiv_index = 0;
    ob->div_index = 0;

    if ( bfish_keyinit(bfkey, klen, &ob->bk) == NULL) {
        log(1, "Error: Could not initiate blowfish key!\n");
        uland_free(ob);
        return(NULL);
    }

    bfish_encrypt((uint32_t *)&ob->enciv[0], (uint32_t *)&ob->enciv[4], &ob->bk);
    bfish_encrypt((uint32_t *)&ob->deciv[0], (uint32_t *)&ob->deciv[4], &ob->bk);
    return(ob);
}

/*
 * Clear IV and key
 */
void
bfish_ofb8_cleariv(struct bfish_ofb *ob)
{
    if (ob == NULL) {
        debug(2, "Error: Received NULL pointer to blowfish "
                "output feedback mode structure\n");
        return;
    }

    memset(ob, 0x00, sizeof(struct bfish_ofb));
    debug(3, "Cleared OFB mode key at address 0x%08x\n", (u_int)ob);
    kspace_free(ob);
}


/*
 * Encrypt string using encryption IV.
 * This is in fact 64 bit mode, but 8 bits at the time.
 */
void
bfish_ofb8_encrypt(uint8_t *str, uint16_t len, struct bfish_ofb *ob)
{
    int i = 0;
    
    while (i < len) {
    
        /* End of IV, encrypt */
        if (ob->eiv_index >= sizeof(ob->enciv)) {
            bfish_encrypt((uint32_t *)&ob->enciv[0], (uint32_t *)&ob->enciv[4], &ob->bk);
            ob->eiv_index = 0;
        }

        str[i++] ^= ob->enciv[ob->eiv_index++];
    }
}

/*
 * Decrypt string using decryption IV.
 * This is in fact 64 bit mode, but 8 bits at the time.
 */
void
bfish_ofb8_decrypt(uint8_t *str, uint16_t len, struct bfish_ofb *ob)
{
    int i = 0;
    
    while (i < len) {

        /* End of IV, encrypt */
        if (ob->div_index >= sizeof(ob->deciv)) {
            bfish_encrypt((uint32_t *)&ob->deciv[0], (uint32_t *)&ob->deciv[4], &ob->bk);
            ob->div_index = 0;
        }

        str[i++] ^= ob->deciv[ob->div_index++];
    }
}
