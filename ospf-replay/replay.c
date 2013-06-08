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
#include "replay.h"
#include "utility.h"


void start_listening();
void load_defaults();
void load_config(const char*);
void process_packet(int);
void add_prefix();
void add_interface(struct ifreq*);

// holds program settings and log files
struct replay_config *replay0;
// holds ospf process, settings, and links to all ospf data structures
struct ospf *ospf0;

int main(int argc, char *argv[])
{
	// reusable boolean variable
	int bool=0;

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
		load_config(argv[2]);
		// reset the boolean
		bool=0;
	}
	else {
		load_config("./replay.config");
	}

	// start listening for OSPF packets
	start_listening();

	return 0;
}

void load_defaults() {

	replay_log("load_defaults: creating globals\n");
	// malloc the two global structs
	replay0 = (struct replay_config *) malloc(sizeof(struct replay_config));
	ospf0 = (struct ospf *) malloc(sizeof(struct ospf));

	replay_log("load_defaults: pointers to NULL\n");
	// set the replay0 FILE pointers to null
	replay0->errors = replay0->events = replay0->lsdb = replay0->packets = NULL;

	replay_log("load_defaults: numeric members to 0\n");
	// set replay0 numeric member variables to 0
	replay0->log_events = replay0->log_packets = replay0->lsdb_history = 0;

	replay_log("load_defaults: OSPF process Router ID to 0.0.0.0\n");
	// initialize the OSPF process router_id to 0.0.0.0
	bzero((char *) &ospf0->router_id, sizeof(ospf0->router_id));
	replay_log("load_defaults: ospf0 numeric members to defaults");
	// initialize the OSPF process numeric members to their defaults
	ospf0->passif = OSPF_DEFAULT_PASSIF;
	ospf0->cost = OSPF_DEFAULT_COST;
	ospf0->dead_interval = OSPF_DEFAULT_DEAD;
	ospf0->hello_interval = OSPF_DEFAULT_HELLO;
	ospf0->retransmit_interval = OSPF_DEFAULT_RETRANSMIT;
	ospf0->transmit_delay = OSPF_DEFAULT_TRANSMITDELAY;


	replay_log("load_defaults: Clearing out the select() file/stream sets\n");
	// clear out the select() file/stream descriptor sets
	FD_ZERO(&ospf0->ospf_sockets_err);
	FD_ZERO(&ospf0->ospf_sockets_in);
	FD_ZERO(&ospf0->ospf_sockets_out);

	replay_log("load_defaults: Adding stdin to select() input set\n");
	// add stdin to the select() input set, this allows us to have a command line which accepts commands while we are looking for packets
	FD_SET(0, &ospf0->ospf_sockets_in);
	ospf0->max_socket = 0;

	replay_log("load_defaults: finished with load_defaults\n");
}

void load_config(const char* filename) {

	// this FILE descriptor points to the config file, normally replay.config
	FILE *config;
	// temporary variable to hold the line of the file that has just been read in
	char line[256];
	// temp variable to hold log/error strings including variables
	char mesg[256] = "";
	// word# holds an individual word from current line of the file, ifname = interface name when in the interface block of the config file
	char* word, word2, word3, word4, ifname, logfilename;
	// indicates which block of the config file is currently being read
	int config_block=0;
	// before we do anything create the global structs and make sure all their necessary members are NULL'd or zero'd out
	load_defaults();

	strcat(mesg,"load_config: Opening config file - ");
	strcat(mesg,filename);
	replay_log(mesg);
	mesg[0]='\0';
	// open the config file for reading
	config = fopen(filename,"r");

	// if the file was successfully opened...
	if(config) {
		replay_log("load_config: File opened");
		while(!feof(config)) {
			if(fgets(line,sizeof(line),config)) {
				word = strtok(line," \n");
				if(!strcmp("logging",word)) {
					config_block = REPLAY_CONFIG_LOGGING;
				}
				else if(!strcmp("router",word)) {
					config_block = REPLAY_CONFIG_ROUTER;
				}
				else if(!strcmp("interface",word)) {
					ifname = strtok(NULL," \n");
					if(ifname) {
						config_block = REPLAY_CONFIG_IF;
					}
				}
				else if(config_block) {

					switch(config_block) {

					case REPLAY_CONFIG_LOGGING:
						if(!strcmp("file",word)) {
							word = strtok(NULL," \n");
							if(word) {
								logfilename = strtok(NULL," \n");
								if(logfilename) {
									if(!strcmp("all",word)) {
										replay0->packets = replay0->events = replay0->errors = fopen(logfilename,"a");
										if(!replay0->packets) {
											replay_error("file error: unable to open/create log specified in config file\n");
										}
									}
									else if(!strcmp("events",word)) {
										replay0->events = fopen(logfilename,"a");
										if(!replay0->events) {
											replay_error("file error: unable to open/create events log specified in config file\n");
										}
									}
									else if(!strcmp("packets",word)) {
										replay0->packets = fopen(logfilename,"a");
										if(!replay0->packets) {
											replay_error("file error: unable to open/create packets log specified in config file\n");
										}
									}
									else if(!strcmp("errors",word)) {
										replay0->errors = fopen(logfilename,"a");
										if(!replay0->errors) {
											replay_error("file error: unable to open/create errors log specified in config file\n");
										}
									}
									else if(!strcmp("lsdb",word)) {
										replay0->lsdb = fopen(logfilename,"a");
										if(!replay0->lsdb) {
											replay_error("file error: unable to open/create lsdb log specified in config file\n");
										}
									}
								}
								else {
									replay_error("config file error: log block - missing log filename\n");
								}
							}
							else {
								replay_error("config file error: log block - missing file type (all, events, packets, errors, lsdb)\n");
							}
						}
						else if(!strcmp("packets",word)) {
							word = strtok(NULL," \n");
							while((word) || (replay0->log_packets < 255)) {
								if(!strcmp("all",word)) {
									replay0->log_packets = REPLAY_PACKETS_ALL;
								}
								else if(!strcmp("hello",word)) {
									replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_HELLO;
								}
								else if(!strcmp("dbdesc",word)) {
									replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_DBDESC;
								}
								else if(!strcmp("lsr",word)) {
									replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSR;
								}
								else if(!strcmp("lsu",word)) {
									replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSU;
								}
								else if(!strcmp("lsack",word)) {
									replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSACK;
								}
								word = strtok(NULL," \n");
							}
							if(replay0->log_packets>255) {
								replay_error("config error: log block - packet logging set to invalid value");
								replay0->log_packets = REPLAY_PACKETS_NONE;
							}
						}
						else if(!strcmp("events",word)) {
							word = strtok(NULL," \n");
							while((word) || (replay0->log_events < 255)) {
								if(!strcmp("all",word)) {
									replay0->log_events = REPLAY_EVENTS_ALL;
								}
								else if(!strcmp("adj-change",word)) {
									replay0->log_events = replay0->log_events + REPLAY_EVENTS_ADJ;
								}
								else if(!strcmp("if-change",word)) {
									replay0->log_events = replay0->log_events + REPLAY_EVENTS_IF;
								}
								else if(!strcmp("spf",word)) {
									replay0->log_events = replay0->log_events + REPLAY_EVENTS_SPF;
								}
								else if(!strcmp("auth",word)) {
									replay0->log_events = replay0->log_events + REPLAY_EVENTS_AUTH;
								}
								word = strtok(NULL," \n");
							}
							if(replay0->log_events>255) {
								replay_error("config error: log block - event logging set to invalid value");
								replay0->log_events = REPLAY_EVENTS_NONE;
							}
						}
						else if(!strcmp("lsdb-history",word)) {
							word = strtok(NULL," \n");
							if(word) {
								replay0->lsdb_history = word[0] - '0';
							}
							else {
								replay_error("config error: log block - no LSDB history number provided");
							}
						}

						break;

					case REPLAY_CONFIG_ROUTER:
						// load router configuration
						if(!strcmp("router_id",word)) {
							word = strtok(NULL," \n");
							if(word) {
								inet_pton(AF_INET,word,&ospf0->router_id);
							}
						}
						else if(!strcmp("passive-interface",word)) {
							ospf0->passif = 1;
						}
						else if(!strcmp("no",word)) {
							word = strtok(NULL," \n");
							if(word) {
								if(!strcmp("passive-interface",word)) {
									ospf0->passif = 0;
								}
							}
						}
						else if(!strcmp("network",word)) {
							word = strtok(NULL," \n");
							word2 = strtok(NULL," \n");
							word3 = strtok(NULL," \n");
							if(word && !strcmp("area",word2) && word3) {
								add_prefix(word,atoi(word3));
							}
							else {
								// if the network statement is missing an IP/mask, area statement or area number then record an error
								strcat(mesg,"config error: invalid format of line - ");
								strcat(mesg,line);
								replay_error(mesg);
								mesg[0]='\0';
							}
						}
						break;

					case REPLAY_CONFIG_IF:
						// load interface config(s)
						break;

					}

				}

			}
		}
		fclose(config);
		replay_log("load_config: config file closed");
	}
	replay_log("load_config: Setting log file defaults for any unset FILEs");
	if(!replay0->errors) {
		replay0->errors = fopen("errors.log","a");
	}
	if(!replay0->packets) {
		replay0->packets = replay0->errors;
	}
	if(!replay0->events) {
		replay0->events = replay0->errors;
	}
	if(!replay0->lsdb) {
		replay0->lsdb = fopen("lsdb.replay","a");
	}

	replay_log("load_config: Finished loading config");
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

void process_packet(int socket) {

}

void add_prefix(char* full_string, u_int32_t area) {

	char net_string[16],mask_string[16];
	struct ifreq *iface;
	struct ifconf ifc;
	struct in_addr net, mask;
	struct ospf_prefix *new_pfx, *tmp_pfx;
	int duplicate = FALSE;

	// split up the network and mask
	net_string = strtok(full_string,"/");
	mask_string = strtok(NULL,"/");

	mask->s_addr = bits2mask(atoi(mask_string));

	inet_pton(AF_INET,net_string,net.s_addr);

	tmp_pfx = ospf0->pflist;
	while(tmp_pfx) {
		if((tmp_pfx->network.s_addr == net.s_addr) && (tmp_pfx->mask->s_addr == mask.s_addr)) {
			duplicate = TRUE;
			tmp_pfx = NULL;
		}
		else {
			tmp_pfx = tmp_pfx->next;
		}
	}
	if(!duplicate) {
		iface = find_interface(net.s_addr,mask.s_addr);

		new_pfx = (struct ospf_prefix *) malloc(sizeof(struct ospf_prefix));

		new_pfx->mask.s_addr = mask.s_addr;
		new_pfx->network.s_addr = net.s_addr;
		new_pfx->next = NULL;
		new_pfx->iface = iface;
		new_pfx->ospf_if = NULL;

		ospf0->pflist = add_to_list(ospf0->pflist,new_pfx);

		if(iface && !ospf0->passif) {
			new_pfx->ospf_if = add_interface(iface, area);
			if(new_pfx->ospf_if) {

				duplicate = FALSE;
				tmp_pfx = new_pfx->ospf_if->pflist;
				while(tmp_pfx) {
					if((tmp_pfx->network->s_addr == new_pfx->network->s_addr) && (tmp_pfx->mask->s_addr == new_pfx->network->s_addr)) {
						duplicate = FALSE;
						tmp_pfx = NULL;
					}
					else {
						tmp_pfx = tmp_pfx->next;
					}
				}
				if(!duplicate) {
					new_pfx->ospf_if->pflist = add_to_list(new_pfx->ospf_if->pflist,new_pfx);
				}
			}
		}
	}
}

struct ifreq* find_interface(uint32_t net_addr, uint32_t mask_addr) {

	struct ifreq *ifr, *found;
	struct ifconf ifc;
	int s, rc, i;
	int numif;
	uint32_t if_net;

	found = NULL;

	// find number of interfaces.
	memset(&ifc,0,sizeof(ifc));
	ifc.ifc_ifcu.ifcu_req = NULL;
	ifc.ifc_len = 0;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		replay_error("find_interface: unable to open socket");
	}

	if ((rc = ioctl(s, SIOCGIFCONF, &ifc)) < 0) {
		replay_error("find_interface: error on 1st ioctl");
	}

	numif = ifc.ifc_len / sizeof(struct ifreq);
	if ((ifr = malloc(ifc.ifc_len)) == NULL) {
		replay_error("find_interface: error on malloc of ifreq");
	}
	ifc.ifc_ifcu.ifcu_req = ifr;

	if ((rc = ioctl(s, SIOCGIFCONF, &ifc)) < 0) {
		replay_error("find_interface: error on 2nd ioctl");
	}

	close(s);

	for(i = 0; i < numif; i++) {

		struct ifreq *r = &ifr[i];
		struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
		if_net = get_net(sin->sin_addr->s_addr,mask_addr);
		if (if_net == net_addr) {
			found = r;
		}
	}
	return found;
}

struct ospf_interface* add_interface(struct ifreq *iface, u_int32_t area) {

	struct ospf_interface *new_if, *tmp_if;
	struct ip_mreq mreq;
	int ospf_socket;
	struct sockaddr_in *sin = (struct sockaddr_in *)&iface->ifr_addr;
	char ifname[16];
	int duplicate=FALSE;

	strcpy(ifname,iface->ifr_ifrn->ifrn_name);

	tmp_if = ospf0->iflist;
	while(tmp_if) {
		if(!strcmp(ifname,tmp_if->iface->ifr_ifrn->ifrn_name)) {
			duplicate = TRUE;
			new_if = tmp_if;
			tmp_if = NULL;
		}
		else {
			tmp_if = tmp_if->next;
		}
	}

	if(!duplicate) {
		new_if = (struct ospf_interface *) malloc(sizeof(struct ospf_interface));

		new_if->area_id = area;
		new_if->iface = iface;
		new_if->next = NULL;
		new_if->pflist = NULL;

		ospf0->iflist = add_to_list(ospf0->iflist,new_if);

		ospf_socket = socket(AF_INET,SOCK_RAW,89);

		if(ospf_socket<0)
			replay_error("add_interface: Error opening socket");
		else
			replay_log("add_interface: socket opened");

		if (setsockopt(ospf_socket, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface)) < 0)
			replay_error("add_interface: ERROR on interface binding");
		else
			replay_log("add_interface: socket bound to interface");

		mreq.imr_interface = sin->sin_addr->s_addr;
		inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&mreq.imr_multiaddr);

		if(setsockopt(ospf_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))<0)
			replay_error("add_interface: ERROR on multicast join");
		else
			replay_log("add_interface: multicast group added");

		FD_SET(ospf_socket, ospf0->ospf_sockets_in);
		FD_SET(ospf_socket, ospf0->ospf_sockets_out);
		FD_SET(ospf_socket, ospf0->ospf_sockets_err);
	}
	return new_if;

}

void remove_inteface(struct ospf_interface *ospf_if) {


}

void remove_prefix(struct ospf_prefix *ospf_pfx) {


}
