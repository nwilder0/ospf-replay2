/*
 * command.c
 *
 *  Created on: Sep 8, 2013
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
#include "command.h"
#include "replay.h"


int process_command(char *buffer) {

	int run_replay=1;
	char *word[10];

	word[0] = strtok(buffer,WHITESPACE);
	if(!strcmp("quit",word[0])) {
		run_replay=0;
	} else if(!strcmp("show",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		word[2] = strtok(NULL,WHITESPACE);
		word[3] = strtok(NULL,WHITESPACE);
		if(word[1]) {
			if(!strcmp("ip",word[1])) {
				if(word[2]) {
					if(!strcmp("ospf",word[2])) {
						if(word[3]) {
							if(!strcmp("neighbors",word[3])) {
								show_nbrs();
								printf(">");
								fflush(stdout);
							}
						}
					}
				}
			}
		}
	} else if(!strcmp("debug",word[0])) {
		// debug commands
	} else if(!strcmp("write",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		word[2] = strtok(NULL,WHITESPACE);
		if(word[1]) {
			if(!strcmp("lsdb",word[1])) {
				write_lsdb(word[2]);
				printf("LSDB written to disk\n");
				printf(">");
				fflush(stdout);
			}
		}
	} else if(!strcmp("read",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		word[2] = strtok(NULL,WHITESPACE);
		if(word[1]) {
			if(!strcmp("lsdb",word[1])) {
				read_lsdb(word[2]);
				printf("LSDB read from disk\n");
				printf(">");
				fflush(stdout);
			}
		}
	} else {
		printf(">");
		fflush(stdout);
	}

	return run_replay;
}

void show_nbrs() {
	struct ospf_neighbor *nbr;
	struct replay_list *item;
	char rtr_id_str[INET_ADDRSTRLEN],iface_ip_str[INET_ADDRSTRLEN],nbr_ip_str[INET_ADDRSTRLEN];
	char nbr_state_str[32];

	nbr_state_str[0] = '\0';
	//print headers
	printf("Neighbor ID          State               Dead Time     Address              Interface\n");
	item = ospf0->nbrlist;
	while(item) {
		nbr = item->object;
		if(nbr) {

			switch(nbr->state) {
				case OSPF_NBRSTATE_DOWN:
					strcpy(nbr_state_str,"Down");
					break;
				case OSPF_NBRSTATE_ATTEMPT:
					strcpy(nbr_state_str,"Attempt");
					break;
				case OSPF_NBRSTATE_INIT:
					strcpy(nbr_state_str,"Initial");
					break;
				case OSPF_NBRSTATE_2WAY:
					strcpy(nbr_state_str,"2-Way");
					break;
				case OSPF_NBRSTATE_EXSTART:
					strcpy(nbr_state_str,"ExStart");
					break;
				case OSPF_NBRSTATE_EXCHANGE:
					strcpy(nbr_state_str,"Exchange");
					break;
				case OSPF_NBRSTATE_LOADING:
					strcpy(nbr_state_str,"Loading");
					break;
				case OSPF_NBRSTATE_FULL:
					strcpy(nbr_state_str,"Full");
					break;
			}
			inet_ntop(AF_INET, &nbr->router_id, rtr_id_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &nbr->ip, nbr_ip_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &nbr->ospf_if->iface->ip, iface_ip_str, INET_ADDRSTRLEN);

			printf("%-21s%-21s%-13fs%-21s%-4s:%-21s\n",rtr_id_str,nbr_state_str,(double)(nbr->last_heard.tv_sec),nbr_ip_str,(nbr->ospf_if->iface->name),iface_ip_str);
		}
		item = item->next;
	}

}

