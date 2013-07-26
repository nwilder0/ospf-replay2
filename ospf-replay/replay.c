/*
 * replay.c
 *
 *  Created on: May 19, 2013
 *      Author: nathan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "utility.h"
#include "replay.h"


void start_listening();

// holds program settings and log files
struct replay_config *replay0;
// holds ospf process, settings, and links to all ospf data structures
struct ospf *ospf0;

int main(int argc, char *argv[])
{
	// reusable boolean variable
	int bool=0;

	// allocate the two global structs
	replay0 = (struct replay_config *) malloc(sizeof(*replay0));
	memset(replay0,0,sizeof(*replay0));
	ospf0 = (struct ospf *) malloc(sizeof(*ospf0));
	memset(ospf0,0,sizeof(*ospf0));

	//printf("\nSetEUID: %i",seteuid(0));
	printf("\nGetUID: %i",getuid());
	printf("\nGetEUID: %i\n",geteuid());
	// check for the -config parameter
	if(argc>2) {
		if(!strcmp(argv[1],"-config")) {
			bool=1;
		}
		else {
			printf("\nFormat: replay -config [filename]\nProgram assumes 'replay.config' if no config file given\n");
		}
	}
	// if the -config argument was provided then load that config file, otherwise use the default "replay.config"
	if(bool) {
		printf("load_config\n");
		load_config(argv[2]);
		// reset the boolean
		bool=0;
	}
	else {
		printf("load_config\n");
		load_config("/tmp/replay.config");
	}
	printf("start_listening\n");
	set_router_lsa();

	// start listening for OSPF packets
	ospf0->started = 1;
	start_listening();
	ospf0->started = 0;

	unload_replay();
	return 0;
}


void start_listening() {

	fd_set sockets_in, sockets_out, sockets_err;
	struct timeval tv;
	int bool=0;
	int max_socket, i;
	char buffer[256];

	printf("\n>");
	fflush(stdout);
	max_socket = ospf0->max_socket;
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	bool = 1;
	while(bool) {

		sockets_in = ospf0->ospf_sockets_in;
		sockets_out = ospf0->ospf_sockets_out;
		sockets_err = ospf0->ospf_sockets_err;
		check_events();
		if(select(max_socket+1,&sockets_in,&sockets_out,&sockets_err,&tv)==-1) replay_error("socket error: unable to run select()");


		for(i = 0; i<=max_socket; i++) {
			if(FD_ISSET(i, &sockets_in)) {
				if(!i) {
					fgets(buffer,sizeof(buffer),stdin);
					if(!strcmp("quit\n",buffer)) {
						bool=0;
					}
					else {
						printf(">");
						fflush(stdout);
					}
				}
				else {

					process_packet(i);

				}
			}

		}



	}
}

void recalc_max_socket() {
	struct replay_list *tmp_item;
	struct ospf_interface *tmp_if;

	ospf0->max_socket = 0;

	tmp_item = ospf0->iflist;
	while(tmp_item) {
		tmp_if = (struct ospf_interface *)tmp_item->object;
		if(tmp_if) {
			if(tmp_if->ospf_socket > ospf0->max_socket) {
				ospf0->max_socket = tmp_if->ospf_socket;
			}
		}
		tmp_item = tmp_item->next;
	}

}
