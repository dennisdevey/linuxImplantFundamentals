/*
 *  File: sadb.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor database routines
 *  Version: 1.0
 *  Date: Mon Mar 17 20:11:03 CET 2003
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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "net.h"
#include "utils.h"
#include "sadb.h"


/*
 * Find entry in opened file by its IP address.
 * The pointer returned/set at ent points into the memory-mapped file,
 * so it is only available as long as the file is opened.
 * NULL is returned on error.
 */
struct sadbent *
sadb_getentip(const struct mfile *mf, u_int ipaddr, struct sadbent *ent)
{
	int size;	
	u_char *pt; 
	
	pt = mf->mf_file;
	size = mf->mf_size;
	
	while (size > 0) {

		if (sadb_genent(pt, size, ent) == NULL) 
			return(NULL);	
		
		if (ent->se_hdr->sah_ipv4addr == ipaddr) 
			return(ent);

		size -= ent->se_hdr->sah_totlen;
		pt += ent->se_hdr->sah_totlen;
	}
	
	fprintf(stderr, "Entry not found!\n");
	return(NULL);
}


/*
 * Atemmpts to generate a sadb entry from
 * the data pointed to by data, Returns NULL on error.
 */
struct sadbent *
sadb_genent(u_char *indata, u_int dlen, struct sadbent *ent)
{
	u_int len;
	u_char *data = indata;
	u_char *ent_name;
	u_int pcount;
	struct sa_pkt *pkt;

	if (data == NULL) {
		fprintf(stderr, "** Error: sadb_genent(): Received NULL "
			"pointer as data\n");
		return(NULL);
	}

	if (dlen <= sizeof(struct sadbhdr)) {
		fprintf(stderr, "** Error: sadb_genent(): Data length is to "
			"short (%u bytes)\n", dlen);
		return(NULL);
	}

	/* Get header */
	ent->se_hdr = (struct sadbhdr *)data;
	ent->se_hdr->sah_totlen = (u_int)ntohl(ent->se_hdr->sah_totlen);
	len = ent->se_hdr->sah_totlen;

	if ((len < sizeof(struct sadbhdr)) || (len > dlen)) {
		fprintf(stderr, "** Error: sadb_genent(): Bad entry length "
			"in header (%u)\n", len);
		return(NULL);
	}

	/* Convert header values to host endian */
	ent->se_hdr->sah_btime = (u_int)(ntohl(ent->se_hdr->sah_btime));
	ent->se_hdr->sah_tout = (u_int)(ntohl(ent->se_hdr->sah_tout));
	ent->se_hdr->sah_pktcount = (u_int)(ntohl(ent->se_hdr->sah_pktcount));

	pcount = ent->se_hdr->sah_pktcount;
	len -= sizeof(struct sadbhdr);
	data += sizeof(struct sadbhdr);

	if (len < sizeof(struct sa_pkt)) {
		fprintf(stderr, "** Error: sadb_genent(): Length is shorter "
			"than one packet");
		return(NULL);
	}
	
	/* First key packet */
	len -= sizeof(struct sa_pkt);
	ent->se_kpkts = (struct sa_pkt *)data;
	pkt = ent->se_kpkts;
	pkt->sa_pkt_proto = ntohs(pkt->sa_pkt_proto);
	data += sizeof(struct sa_pkt);

	/* Save IP for error messages */
	ent_name = net_ntoa(ent->se_hdr->sah_ipv4addr, NULL);

	/* pkt always point to the current packet at top of loop */
	while ((pcount > 0) && (pkt != NULL) && (len > 0)) {
		u_int proto_dlen = 0;	/* Length of protocol data */
		u_char err = 0;
		u_int pktnum = ent->se_hdr->sah_pktcount - pcount +1;

		/* Get protocol */
		pkt->sa_pkt_ph = (void *)data;
		switch(pkt->sa_pkt_proto) {

			case SAPKT_PROTO_TCP: 
				if (len < sizeof(struct sa_tcph)) {
					err++;	
					break;
				}
				len -= sizeof(struct sa_tcph);
				((struct sa_tcph *)data)->sa_tcph_dlen = ntohs(((struct sa_tcph *)data)->sa_tcph_dlen);
				proto_dlen = ((struct sa_tcph *)data)->sa_tcph_dlen;
				((struct sa_tcph *)data)->sa_tcph_data = data + sizeof(struct sa_tcph);
				data += sizeof(struct sa_tcph);
				break;
			
			case SAPKT_PROTO_UDP: 
				if (len < sizeof(struct sa_udph)) {
					err++;
					break;
				}
				len -= sizeof(struct sa_udph);
				((struct sa_udph *)data)->sa_udph_dlen = ntohs(((struct sa_udph *)data)->sa_udph_dlen);
				proto_dlen = ((struct sa_udph *)data)->sa_udph_dlen;
				((struct sa_udph *)data)->sa_udph_data = data + sizeof(struct sa_udph);
				data += sizeof(struct sa_udph);
				break;

			case SAPKT_PROTO_ICMP: 
				if (len < sizeof(struct sa_icmph)) {
					err++;
					break;
				}
				len -= sizeof(struct sa_icmph);
				((struct sa_icmph *)data)->sa_icmph_dlen = ntohs(((struct sa_icmph *)data)->sa_icmph_dlen);
				proto_dlen = ((struct sa_icmph *)data)->sa_icmph_dlen;
				((struct sa_icmph *)data)->sa_icmph_data = data + sizeof(struct sa_icmph);
				data += sizeof(struct sa_icmph);
				break;

			default:
				fprintf(stderr, "** Error in entry %s: Unrecognized protocol "
						"(0x%04x) in packet %u\n", ent_name, pkt->sa_pkt_proto, pktnum);
				return(NULL);
				break;

		}

		if ((err != 0) || (len < (proto_dlen + SA_ALIGN(proto_dlen, SADB_ALIGN)))) {
			fprintf(stderr, "** Error: Protocol data larger than remaining space "
				"(%u/%u packet %u of entry %s)\n" ,len, proto_dlen, pktnum, ent_name);
			return(NULL);
		}
		data += (proto_dlen + SA_ALIGN(proto_dlen, SADB_ALIGN));
		len -= (proto_dlen + SA_ALIGN(proto_dlen, SADB_ALIGN));
		pcount--;

		if (pcount == 0)
			break;

		/* Next packet */
		if (len < sizeof(struct sa_pkt)) {
			fprintf(stderr, "** Error in entry %s: Data ended prematurely", 
				ent_name);
			return(NULL);
		}
	
		/* Next packet is the command packet */
		if (pcount == 1) {

			if (pkt->sa_kpkt_next != NULL) {
				fprintf(stderr, "** Error in entry %s: Last key packet does not have "
					" NULL pointer as \"next\"\n", ent_name);
				return(NULL);
			}
			ent->se_cpkt = (struct sa_pkt *)data;		
		}
		else 
			pkt->sa_kpkt_next = (struct sa_pkt *)data;
		
		pkt = (struct sa_pkt *)data;
		pkt->sa_pkt_proto = ntohs(pkt->sa_pkt_proto);

		data += sizeof(struct sa_pkt);
		len -= sizeof(struct sa_pkt);
	}
	return(ent);
}

/*
 * Print entry pointed to by ent in "human-readable" format.
 * If resolve is 1, all IP addressed is converted to hostnames.
 * If packets is zero, packets are not printed.
 */
void
sadb_printent(struct sadbent *ent, int resolve, u_char ppackets, u_char pkey)
{
	struct sa_pkt *pkt;
	u_int pktnum;
	u_char *pt;
	
	if (ent == NULL) {
		fprintf(stderr, "** Error: Received NULL pointer as sadb entry\n");
		return;
	}

	/* Print header */
	//printf(" Entry size: %u bytes\n", ent->se_hdr->sah_totlen);
	printf("       Host: %s", net_ntoa(ent->se_hdr->sah_ipv4addr, NULL));

	if (resolve == 1) {
		if ( (pt = net_hostname2(ent->se_hdr->sah_ipv4addr)) != NULL)
			printf(" (%s)", pt);
	}

	/* Print host key */
	if (pkey) {
		u_int i;
		printf("\n   Host key: ");

		for (i=0; i< sizeof(ent->se_hdr->sah_bfkey); i++)
			printf("%02x", (u_char)ent->se_hdr->sah_bfkey[i]);
	}

	printf("\n    Version: %s\n", ent->se_hdr->sah_ver);
	printf("       Time: %s\n", timestr(ent->se_hdr->sah_btime, NULL));
	printf("         OS: %s %s (%s)\n", ent->se_hdr->host.sah_os,
		ent->se_hdr->host.sah_rel, ent->se_hdr->host.sah_mah);

	printf("    Promisc: %s\n", 
		PROMISC(ent->se_hdr->sah_flags) ? "yes" : "no");
	printf(" Single cmd: %s\n", 
		NOSINGLE(ent->se_hdr->sah_flags) ? "no" : "yes");
	printf(" Accept cmd: %s\n",
		NOACCEPT(ent->se_hdr->sah_flags) ? "no" : "yes");
	printf("Connect cmd: %s\n",
		NOCONNECT(ent->se_hdr->sah_flags) ? "no" : "yes");
	
	printf("    Timeout: ");

	if (ent->se_hdr->sah_tout == 0)
		printf("Not set\n");
	else
		printf("%u sec.\n", ent->se_hdr->sah_tout);
	
	if (ppackets == 0) {
		printf("\n");
		return;
	}

	/* Print packets */
	pkt = ent->se_kpkts;
	pktnum = 1;
	
	while (pkt != NULL) {
		u_int dlen = 0;
		u_char *data = NULL;

		if (pkt == ent->se_cpkt)
			printf("\nCommand packet:\n");
		else	
			printf("\nKey Packet %u:\n", pktnum);
		
		/* Write IP */
		if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_DADDR)) {
			printf("   Dest address: %s", net_ntoa(pkt->sa_iph_daddr, NULL));
			if (resolve == 1) {
				if ( (pt = net_hostname2(pkt->sa_iph_daddr)) != NULL)
					printf(" (%s)", pt);
			}
			printf("\n");
		}

		if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_SADDR)) {
			printf(" Source address: %s", net_ntoa(pkt->sa_iph_saddr, NULL));
			if (resolve == 1) {
				if ( (pt = net_hostname2(pkt->sa_iph_saddr)) != NULL)
					printf(" (%s)", pt);
			}
			printf("\n");
		}

		if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_TOS))
			printf("Type of service: 0x%02x\n", pkt->sa_iph_tos);

		if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_ID))
			printf("        IPv4 ID: 0x%04x\n", htons(pkt->sa_iph_id));

		if (FIELD_IS_SET(pkt->sa_iph_fields, IPH_TTL))
			printf("   Time To Live: %u\n",pkt->sa_iph_ttl);

		/* Print protocol */
		if (pkt->sa_pkt_proto == SAPKT_PROTO_TCP) {
			struct sa_tcph *tph = (struct sa_tcph *)pkt->sa_pkt_ph;

			printf("       Protocol: TCP\n");

			if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_DPORT))
				printf("      Dest port: %u\n", ntohs(tph->sa_tcph_dport));

			if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_SPORT))
				printf("    Source port: %u\n", ntohs(tph->sa_tcph_sport));

			if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_SEQ))
				printf("       Sequence: 0x%08x\n", ntohl((u_int)tph->sa_tcph_seq));

			if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_ACK))
				printf("    Acknowledge: 0x%08x\n", ntohl((u_int)tph->sa_tcph_ack));

			if (FIELD_IS_SET(tph->sa_tcph_fields, TCPH_FLAGS))
				printf("          Flags: %s\n", net_tcpflags(tph->sa_tcph_flags));

			if (tph->sa_tcph_dlen != 0) {
				dlen = tph->sa_tcph_dlen;
				data = tph->sa_tcph_data;
			}
		}
		else if (pkt->sa_pkt_proto == SAPKT_PROTO_UDP) {
			struct sa_udph *uph = (struct sa_udph *)pkt->sa_pkt_ph;

			printf("       Protocol: UDP\n");

			if (FIELD_IS_SET(uph->sa_udph_fields, UDPH_DPORT))
				printf("      Dest port: %u\n", ntohs(uph->sa_udph_dport));

			if (FIELD_IS_SET(uph->sa_udph_fields, UDPH_SPORT))
				printf("    Source port: %u\n", ntohs(uph->sa_udph_sport));

			if (uph->sa_udph_dlen != 0) {
				dlen = uph->sa_udph_dlen;
				data = uph->sa_udph_data;
			}
		}
		else if (pkt->sa_pkt_proto == SAPKT_PROTO_ICMP) {
			struct sa_icmph *ich = (struct sa_icmph *)pkt->sa_pkt_ph;

			printf("       Protocol: ICMP\n");

			if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_TYPE))
				printf("           Type: %u (%s)\n", ich->sa_icmph_type,
					ich->sa_icmph_type == 8 ? "echo request" : "echo reply");

			if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_CODE))
				printf("           Code: %u\n", ich->sa_icmph_code);

			if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_ID))
				printf("             ID: %u\n", ntohs(ich->sa_icmph_id));

			if (FIELD_IS_SET(ich->sa_icmph_fields, ICMPH_SEQ))
				printf("       Sequence: %u\n", ntohs(ich->sa_icmph_seq));
			
			if (ich->sa_icmph_dlen != 0) {
				dlen = ich->sa_icmph_dlen;
				data = ich->sa_icmph_data;
			}	
		}
		else {
			fprintf(stderr, "** Error: Bad procol id: 0x%04x\n",
				pkt->sa_pkt_proto);
			return;
		}
		
		/* Write data */
		if (dlen != 0) {
			int i;

			printf("           Data: ");
			for (i=0; i<dlen; i++) {
				switch(*data) {
                    case '\e': printf("\\e"); break;
                    case '\a': printf("\\a"); break;
                    case '\b': printf("\\b"); break;
                    case '\f': printf("\\f"); break;
                    case '\n': printf("\\n"); break;
                    case '\r': printf("\\r"); break;
                    case '\t': printf("\\t"); break;
                    case '\v': printf("\\v"); break;
                    /* case ' ':  printf("\\s"); break; */
                    default:
                        if (BYTETABLE[*data] == '.')
                            printf("\\x%02x", *data);
                        else
                            printf("%c", *data);
                        break;
                }
                data++;
            }
            printf("\n");
        }
		
		/* Next packet for shaving */
		if (pkt == ent->se_cpkt)
			pkt = NULL;
		else if (pkt->sa_kpkt_next == NULL)
			pkt = ent->se_cpkt;
		else
			pkt = pkt->sa_kpkt_next;
			
		pktnum++;
	}
}

/*
 * Write entry in raw mode to fd.
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
