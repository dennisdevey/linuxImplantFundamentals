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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "bfish.h"

/* Private variables */
static struct bfish_key *bk;
static uint8_t enciv[8];
static uint8_t eiv_index = 0;
static uint8_t deciv[8];
static uint8_t div_index = 0;

/*
 * Initiate Output Feedback Mode.
 * bfkey - The blowfish key
 * klen  - Length of bfkey in bytes
 * eiv - The 8 byte long IV to be used for encryption.
 * div - The 8 byte long IV to be used for decryption.
 */
int
bfish_ofb8_setiv(uint8_t *bfkey, uint8_t klen, uint8_t *eiv, uint8_t *div)
{
	memcpy(enciv, eiv, sizeof(enciv));
	memcpy(deciv, div, sizeof(deciv));
	eiv_index = div_index = 0;

	if (bk != NULL)
		free(bk);

	if ( (bk = bfish_keyinit(bfkey, klen)) == NULL) {
		fprintf(stderr, "** Error: Could not initiate blowfish key!\n");
		return(-1);
	}

	bfish_encrypt((uint32_t *)&enciv[0], (uint32_t *)&enciv[4], bk);
	bfish_encrypt((uint32_t *)&deciv[0], (uint32_t *)&deciv[4], bk);
	return(0);
}

/*
 * Clear IV and key
 */
void
bfish_ofb8_cleariv(void)
{
	if (bk != NULL) {
		memset(bk, 0x00, sizeof(struct bfish_key));
		free(bk);
	}
	bk = NULL;
	memset(enciv, 0x00, sizeof(enciv));
	memset(deciv, 0x00, sizeof(deciv));
}


/*
 * Encrypt string using encryption IV.
 * This is in fact 64 bit mode, but 8 bits at the time.
 */
void
bfish_ofb8_encrypt(uint8_t *str, uint16_t len)
{
	int i = 0;
	
	while (i < len) {
	
		/* End of IV, encrypt */
		if (eiv_index >= sizeof(enciv)) {
			bfish_encrypt((uint32_t *)&enciv[0], (uint32_t *)&enciv[4], bk);
			eiv_index = 0;
		}

		str[i++] ^= enciv[eiv_index++];
	}
}

/*
 * Decrypt string using decryption IV.
 * This is in fact 64 bit mode, but 8 bits at the time.
 */
void
bfish_ofb8_decrypt(uint8_t *str, uint16_t len)
{
	int i = 0;
	
	while (i < len) {

		/* End of IV, encrypt */
		if (div_index >= sizeof(deciv)) {
			bfish_encrypt((uint32_t *)&deciv[0], (uint32_t *)&deciv[4], bk);
			div_index = 0;
		}

		str[i++] ^= deciv[div_index++];
	}
}
