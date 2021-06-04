/*
 *  File: capture.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor pcap routines.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pcap.h>
#include "capture.h"
#include "sadoor.h"

/* Global options */
extern struct sa_options opt;

/*
 * Opens a device to capture packets from (NULL for lookup).
 * Returns a NULL pointer on error and a pointer
 * to a struct capture on success.
 * Arguments:
 *  dev     - Device to open (NULL for lookup)
 *  promisc - Should be one for open in promisc mode, 0 otherwise
 */
struct capture *
cap_open(u_char *dev, int promisc)
{
    u_char err[PCAP_ERRBUF_SIZE];   /* Pcap error string */
    struct capture *cap; 
	
    if ( (cap = (struct capture *)calloc(1, sizeof(struct capture))) == NULL) {
        fprintf(stderr, "cap_open(): calloc(): %s\n", strerror(errno));
        return(NULL);
    }
    
    /* Let pcap pick an interface to listen on */
    if (dev == NULL) {
        if ( (opt.sao_iface = pcap_lookupdev(err)) == NULL) {
            fprintf(stderr, "%s\n", err);
            return(NULL);
        }
    }

    /* Init pcap */
    if (pcap_lookupnet(opt.sao_iface, &cap->cap_net, 
			&cap->cap_mask, err) != 0) {
        fprintf(stderr, "%s\n", err);
        return(NULL);
    }

    /* Open the interface */
    if ( (cap->cap_pcapd = pcap_open_live(opt.sao_iface, 
            CAP_SNAPLEN, promisc, CAP_TIMEOUT, err)) == NULL) {
        fprintf(stderr, "%s\n", err);
        return(NULL);
    }

    /* Set linklayer offset */
    switch(pcap_datalink(cap->cap_pcapd)) {
        
        case DLT_EN10MB:
            cap->cap_offst = 14;
            break;

        case DLT_PPP:
        case DLT_NULL:
            cap->cap_offst = 4;
            break;

        case DLT_RAW:
            cap->cap_offst = 0;
            break;

        case DLT_SLIP:
            cap->cap_offst = 16;
            break;

        case DLT_SLIP_BSDOS:
        case DLT_PPP_BSDOS:
            cap->cap_offst = 24;
            break;

        case DLT_ATM_RFC1483:
            cap->cap_offst = 8;
            break;

        case DLT_IEEE802:
            cap->cap_offst = 22;
            break;

        default:
            fprintf(stderr, "Unknown datalink type received for iface %s\n", 
				opt.sao_iface);
			free(cap);
            return(NULL);
    }

    return(cap);
}

/*
 * Set capture filter.
 * Returns -1 on error and 1 on success.
 */
int
cap_setfilter(struct capture *cap, u_char *filter)
{
    struct bpf_program fp; /* Holds compiled program */

    /* Compile filter string into a program and optimize the code */
    if (pcap_compile(cap->cap_pcapd, &fp, filter, 1, cap->cap_net) == -1) {
        log_sys("Error compiling filter string!\n");
        return(-1);
    }

    /* Set filter */
    if (pcap_setfilter(cap->cap_pcapd, &fp) == -1) {
        log_sys("Error setting pcap filter!\n");
        return(-1);
    }

	log_priv("Pcap filter: %s\n", filter);
    return(1);
}

