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

	replay0 = (struct replay_config *) malloc(sizeof(struct replay_config));
	ospf0 = (struct ospf *) malloc(sizeof(struct ospf));

	printf("loading defaults\n");
	replay0->errors = replay0->events = replay0->lsdb = replay0->packets = NULL;
	printf("set the ints to 0\n");
	replay0->log_events = replay0->log_packets = replay0->lsdb_history = printf("clear router_id\n");

	bzero((char *) &ospf0->router_id, sizeof(ospf0->router_id));
	printf("clearing sockets\n");
	FD_ZERO(&ospf0->ospf_sockets_err);
	FD_ZERO(&ospf0->ospf_sockets_in);
	FD_ZERO(&ospf0->ospf_sockets_out);
	printf("set stdin\n");
	FD_SET(0, &ospf0->ospf_sockets_in);
	printf("finish loading defaults\n");
}

void load_config(const char* filename) {

	FILE *config;
	char line[256];
	char* word, ifname, logfilename;
	int config_block=0;

	load_defaults();
	printf("loading config\n");
	printf("%s\n",filename);
	config = fopen(filename,"r");
	printf("file opened\n");
	if(config) {
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
					break;

				case REPLAY_CONFIG_IF:
					// load interface config(s)
					break;

				}

			}

		}
	}
	fclose(config);
	}
	printf("file defaults\n");
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

	printf("finish loading config\n");
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
