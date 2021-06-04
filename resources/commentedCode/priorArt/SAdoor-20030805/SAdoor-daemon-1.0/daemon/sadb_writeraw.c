/*
 *  File: sadb_writeraw.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor databse entry writer routines.
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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "sadb.h"

/*
 * Write entry in raw mode to fd, i.e. convert values
 * to network byte order before writing.
 * If verbose is greater that zero, information is printed to stderr.
 */
int
sadb_writeraw(int fd, struct sadbent *ent, int verbose)
{
	struct sadbhdr *sh;
	struct sa_pkt *pkt;
	u_char align_buf[SADB_ALIGN -1];    /* Used for data alignment */

	sh = ent->se_hdr;

	if (verbose > 1) {
		fprintf(stderr, "Entry length: %u bytes\n", sh->sah_totlen);
		fprintf(stderr, "Host IPv4: 0x%08x\n", sh->sah_ipv4addr);
		fprintf(stderr, "Version: %s\n", sh->sah_ver);
		fprintf(stderr, "Time: 0x%08x\n", sh->sah_btime);
		fprintf(stderr, "System: %s %s (%s)\n", sh->host.sah_os,
			sh->host.sah_rel, sh->host.sah_mah);

		fprintf(stderr, "Promisc: %s\n", 
			PROMISC(sh->sah_flags) ? "yes": "no");
		fprintf(stderr, "Single cmd: %s\n",
			NOSINGLE(sh->sah_flags) ? "no": "yes");
		fprintf(stderr, "Accept cmd: %s\n",
			NOACCEPT(sh->sah_flags) ? "no": "yes");
		fprintf(stderr, "Connect cmd: %s\n",
			NOCONNECT(sh->sah_flags) ? "no": "yes");
	
		fprintf(stderr, "Timeout: ");
		if (sh->sah_tout == 0)
			fprintf(stderr, "Disabled\n");
		else
			fprintf(stderr, "%u seconds\n", sh->sah_tout);
		fprintf(stderr,"Packets: %u (%u bytes)\n",
			sh->sah_pktcount, sh->sah_totlen - sizeof(struct sadbhdr));
	}

	/* Header */
	sh->sah_totlen = (u_int)htonl(sh->sah_totlen);
	sh->sah_btime = (u_int)htonl(sh->sah_btime);
	sh->sah_tout = (u_int)htonl(sh->sah_tout);
	sh->sah_pktcount = (int)htonl(sh->sah_pktcount);

	if (write(fd, sh, sizeof(struct sadbhdr)) == -1) {
		perror("write()");
		return(-1);
	}

	pkt = ent->se_kpkts;

	/* Packets */
	while (pkt != NULL) {
		struct sa_pkt pkttmp;

		memcpy(&pkttmp, pkt, sizeof(struct sa_pkt));
		pkt->sa_pkt_proto = htons(pkt->sa_pkt_proto);

		if (write(fd, pkt, sizeof(struct sa_pkt)) == -1) {
			perror("write()");
			return(-1);
		}

		if (pkt->sa_pkt_ph != NULL) {
			
			switch(ntohs(pkt->sa_pkt_proto)) {
				
				case SAPKT_PROTO_TCP:
					{
						struct sa_tcph *tph;
						tph = (struct sa_tcph *)pkt->sa_pkt_ph;
						tph->sa_tcph_dlen = htons(tph->sa_tcph_dlen);

						if (write(fd, tph, sizeof(struct sa_tcph)) == -1) {
							perror("write()");
							return(-1);
						}

						if (tph->sa_tcph_data != NULL) {
                            if (write(fd, tph->sa_tcph_data,
                                    ntohs(tph->sa_tcph_dlen)) == -1) {
                                perror("write()");
                                return(-1);
                            }

                            /* Write alignment */
                            if ( SA_ALIGN(ntohs(tph->sa_tcph_dlen), SADB_ALIGN) != 0) {

                                if (write(fd, align_buf, SA_ALIGN(ntohs(tph->sa_tcph_dlen),
                                        SADB_ALIGN)) == -1) {
                                    perror("write()");
                                    return(-1);
                                }
                            }
                        }
                        if (verbose > 1)
                            fprintf(stderr, "Wrote TCP packet (%u + %u + %u (+ align=%u) bytes)\n",
                                sizeof(struct sa_pkt), sizeof(struct sa_tcph),
                                ntohs(tph->sa_tcph_dlen), SA_ALIGN(ntohs(tph->sa_tcph_dlen), SADB_ALIGN));
					
						/* Swap back to host endian in case this packet is replicated */
						tph->sa_tcph_dlen = ntohs(tph->sa_tcph_dlen);
					}
					break;
					
				case SAPKT_PROTO_UDP:
					{
                        struct sa_udph *uph;
                        uph = (struct sa_udph *)pkt->sa_pkt_ph;
                        uph->sa_udph_dlen = htons(uph->sa_udph_dlen);

                        if (write(fd, uph, sizeof(struct sa_udph)) == -1) {
                            perror("write()");
                            return(-1);
                        }

                        if (uph->sa_udph_data != NULL) {
                            if (write(fd, uph->sa_udph_data,
                                    ntohs(uph->sa_udph_dlen)) == -1) {
                                perror("write()");
                                return(-1);
                            }

                            /* Write alignment */
                            if ( SA_ALIGN(ntohs(uph->sa_udph_dlen), SADB_ALIGN) != 0) {

                                if (write(fd, align_buf, SA_ALIGN(ntohs(uph->sa_udph_dlen),
                                        SADB_ALIGN)) == -1) {
                                    perror("write()");
                                    return(-1);
                                }
                            }

                        }
                        if (verbose > 1)
                            fprintf(stderr, "Wrote UDP packet (%u + %u + %u (+ align=%u) bytes)\n",
                                sizeof(struct sa_pkt), sizeof(struct sa_udph),
                                ntohs(uph->sa_udph_dlen), SA_ALIGN(ntohs(uph->sa_udph_dlen), SADB_ALIGN));
						

						/* Swap back to host endian in case this packet is replicated */
						uph->sa_udph_dlen = ntohs(uph->sa_udph_dlen);
					}
					break;
					
               case SAPKT_PROTO_ICMP:
                    {
                        struct sa_icmph *ich;
                        ich = (struct sa_icmph *)pkt->sa_pkt_ph;
                        ich->sa_icmph_dlen = htons(ich->sa_icmph_dlen);
						
                        if (write(fd, ich, sizeof(struct sa_icmph)) == -1) {
                            perror("write()");
                            return(-1);
                        }

                        if (ich->sa_icmph_data != NULL) {
                            if (write(fd, ich->sa_icmph_data,
                                    ntohs(ich->sa_icmph_dlen)) == -1) {
                                perror("write()");
                                return(-1);
                            }

                            /* Write alignment */
                            if ( SA_ALIGN(ntohs(ich->sa_icmph_dlen), SADB_ALIGN) != 0) {

                                if (write(fd, align_buf, SA_ALIGN(ntohs(ich->sa_icmph_dlen),
                                        SADB_ALIGN)) == -1) {
                                    perror("write()");
                                    return(-1);
                                }
                            }
                        }
                        if (verbose > 1)
                            fprintf(stderr, "Wrote ICMP packet (%u + %u + %u (+ align=%u) bytes)\n",
                                sizeof(struct sa_pkt), sizeof(struct sa_icmph),
                                ntohs(ich->sa_icmph_dlen), SA_ALIGN(ntohs(ich->sa_icmph_dlen), SADB_ALIGN));

						/* Swap back to host endian in case this packet is replicated */
						ich->sa_icmph_dlen = ntohs(ich->sa_icmph_dlen);
                    }
                    break;

                default:
                    fprintf(stderr, "Bad protocol number!\n");
                    return(-1);
                    break;
			}
		}

		if (pkt == ent->se_cpkt)
			break;
		else if (pkt->sa_kpkt_next == NULL)
			pkt = ent->se_cpkt;
		else
			pkt = pkt->sa_kpkt_next;
	}
	return(0);
}
