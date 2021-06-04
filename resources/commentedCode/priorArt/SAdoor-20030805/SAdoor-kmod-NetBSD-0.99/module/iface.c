/*
 *  File: iface.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: Network interface routines
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include "sadoor.h"

#ifdef SAKMOD_IFACE_RUN_PROMISC

/*
 * Put the given interface in promiscous mode
 * We assume net network interrupt is disabled here.
 */
int
iface_promisc(u_char *iface)
{
    struct ifnet *ifp;

    if ( (ifp = ifunit(iface)) == NULL) {
        log(1, "iface_promisc: Couldn't find any interface named %s\n", iface);
        return(1);
    }
    
    ifp->if_flags |= IFF_PROMISC;
    log(1, "Device %s entered promiscous mode\n", iface);
    return(0);
}

/*
 * Remove promiscous mode for the given interface.
 * We assume net network interrupt is disabled here.
 */
int
iface_unpromisc(u_char *iface)
{
    struct ifnet *ifp;

    if ( (ifp = ifunit(iface)) == NULL) {
        log(1, "iface_unpromisc(): Couldn't find any interface named %s\n", iface);
        return(1);
    }

    ifp->if_flags &= ~IFF_PROMISC;
    log(1, "Device %s left promiscous mode\n", iface);
    return(0);
}


#endif /* SAKMOD_IFACE_RUN_PROMISC */

