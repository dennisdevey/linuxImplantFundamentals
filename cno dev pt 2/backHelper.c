#pragma once

/*  File: backHelper.c
*   Author: Nunyabeeswax
*   Date: 6/7/2021
*   Description:
*               - Defines the auxilliary functions declared in backHelper.h
*/

#include "backHelper.h"

#include <unistd.h>
       #include <sys/types.h>
       #include <sys/wait.h>


#ifdef MULTI_KNOCK
    char* key_ports[NUM_PORTS] = {MULTI_KNOCK};
#endif

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){

	static int count = 1;                   /* packet counter */
	static int num_success = 0;				/* Keeps count of successful port knocks */

	/* declare pointers to packet headers */
	const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	const struct sniff_ip *ip;              /* The IP header */
	const struct sniff_tcp *tcp;            /* The TCP header */

	int size_ip;
	int size_tcp;

	#ifdef DEBUG
		printf("\nPacket number %d:\n", count);
	#endif

	count++;

	/* define ethernet header */
	ethernet = (struct sniff_ethernet*)(packet);

	/* define/compute ip header offset */
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		#ifdef DEBUG
			printf("   * Invalid IP header length: %u bytes\n", size_ip);
		#endif
		return;
	}

	/* print source and destination IP addresses */
	#ifdef DEBUG
		printf("       From: %s\n", inet_ntoa(ip->ip_src));
		printf("         To: %s\n", inet_ntoa(ip->ip_dst));
	#endif

	/* determine protocol break if not TCP */
	switch(ip->ip_p) {
		case IPPROTO_TCP:
			#ifdef DEBUG
				printf("   Protocol: TCP\n");
			#endif
			break;
		case IPPROTO_IP:
			#ifdef DEBUG
				printf("   Protocol: IP\n");
			#endif
			return;
		default:
			#ifdef DEBUG
				printf("   Protocol: unknown\n");
			#endif
			return;
	}

	/* define / compute tcp header offset */
	tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
	size_tcp = TH_OFF(tcp)*4;
	if (size_tcp < 20) {
		#ifdef DEBUG
			printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
		#endif
		return;
	}

	#ifdef DEBUG
		printf("   Src port: %d\n", ntohs(tcp->th_sport));
		printf("   Dst port: %d\n", ntohs(tcp->th_dport));
	#endif

	#ifdef SINGLE_KNOCK // TBD
	int prt;
	sscanf(SINGLE_KNOCK,"%d", &prt);
	if( ntohs(tcp->th_dport) == prt){
		switch (fork()) {
			case -1:
				#ifdef DEBUG
				printf("fork() failed!");
				#endif
				return; // end the case (failure)
			case 0: // child process
				singleKnock();
				exit(EXIT_SUCCESS);
				break;
			default: // parent process forced to exit here because can't break from loop
				// if wait() returns failure do the same
				if( wait(NULL) == -1) exit(EXIT_FAILURE);
				else exit(0);
		}
	}
	#endif

	#ifdef MULTI_KNOCK
	int prt;
	sscanf(key_ports[num_success],"%d", &prt);

	if( ntohs(tcp->th_dport) ==  prt){
		num_success++;
		printf("Successful knocks: %d\n", num_success);

		// if theres been as many successes as passed in ports
		if(num_success == (NUM_PORTS)){
			switch (fork()) {
				case -1:
					#ifdef DEBUG
						printf("fork() failed!");
					#endif
					return; // end the case (failure)
				case 0: // child process
					multiKnock();
					exit(EXIT_SUCCESS);
					break;
				default: // parent process forced to exit here because can't break from loop
					// if wait() returns failure do the same
					if( wait(NULL) == -1) exit(EXIT_FAILURE);
					else exit(0);
			}
		}
	}
	#endif
	return;
}


void singleKnock(){
    printf("\nSINGLE KNOCK\n");


}

void multiKnock(){
    printf("\n--------- MULTIPLE KNOCKS -------\n");


}


int getRandom(){
    #ifdef SLEEP
        int slptime = SLEEP;
        //sscanf(SLEEP,"%d", &slptime);
    #else
        int slptime = rand() % 6 + 1;
    #endif
    return slptime;
}

// my implementation of perror(). Checks if DEBUG is defined and calls perror(msg). Returns EXIT_FAILURE (1) always.
int my_perror(char* msg){
    #ifdef DEBUG
    perror(msg);
    #endif
    return EXIT_FAILURE;
}

