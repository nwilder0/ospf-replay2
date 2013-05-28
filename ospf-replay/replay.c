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

int ospf_socket;
struct replay_config *replay;
struct ospf *ospf0;

int main(int argc, char *argv[])
{
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
		bool=0;
	}
	else {
		load_config("replay.config");
	}

	start_listening();

	return 0;
}

void load_defaults() {
	replay->errors = replay->events = replay->lsdb = replay->packets = NULL;
	replay->log_events = replay->log_packets = replay->lsdb_history = 0;


	bzero((char *) &ospf0->router_id, sizeof(ospf0->router_id));

	FD_ZERO(&ospf0->ospf_sockets_err);
	FD_ZERO(&ospf0->ospf_sockets_in);
	FD_ZERO(&ospf0->ospf_sockets_out);

	FD_SET(0, &ospf0->ospf_sockets_in);

}

void load_config(const char* filename) {

	FILE *config;
	char line[256];
	char* word, ifname, logfilename;
	int config_block=0;

	load_defaults();

	config = fopen(filename,"r");
	while(!feof(config)) {
		if(fgets(line,sizeof(line),config)) {
			word = strtok(line," \n");
			if(!config_block) {
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
			}
			else {
				switch(config_block) {

				case REPLAY_CONFIG_LOGGING:
					if(!strcmp("file",word)) {
						word = strtok(NULL," \n");
						if(word) {
							logfilename = strtok(NULL," \n");
							if(logfilename) {
								if(!strcmp("all",word)) {
									replay->packets = replay->events = replay->errors = fopen(logfilename,"a");
									if(!replay->packets) {
										replay_error("file error: unable to open/create log specified in config file\n");
									}
								}
								else if(!strcmp("events",word)) {
									replay->events = fopen(logfilename,"a");
									if(!replay->events) {
										replay_error("file error: unable to open/create events log specified in config file\n");
									}
								}
								else if(!strcmp("packets",word)) {
									replay->packets = fopen(logfilename,"a");
									if(!replay->packets) {
										replay_error("file error: unable to open/create packets log specified in config file\n");
									}
								}
								else if(!strcmp("errors",word)) {
									replay->errors = fopen(logfilename,"a");
									if(!replay->errors) {
										replay_error("file error: unable to open/create errors log specified in config file\n");
									}
								}
								else if(!strcmp("lsdb",word)) {
									replay->lsdb = fopen(logfilename,"a");
									if(!replay->lsdb) {
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
						while((word) || (replay->log_packets < 255)) {
							if(!strcmp("all",word)) {
								replay->log_packets = REPLAY_PACKETS_ALL;
							}
							else if(!strcmp("hello",word)) {
								replay->log_packets = replay->log_packets + REPLAY_PACKETS_HELLO;
							}
							else if(!strcmp("dbdesc",word)) {
								replay->log_packets = replay->log_packets + REPLAY_PACKETS_DBDESC;
							}
							else if(!strcmp("lsr",word)) {
								replay->log_packets = replay->log_packets + REPLAY_PACKETS_LSR;
							}
							else if(!strcmp("lsu",word)) {
								replay->log_packets = replay->log_packets + REPLAY_PACKETS_LSU;
							}
							else if(!strcmp("lsack",word)) {
								replay->log_packets = replay->log_packets + REPLAY_PACKETS_LSACK;
							}
							word = strtok(NULL," \n");
						}
						if(replay->log_packets>255) {
							replay_error("config error: log block - packet logging set to invalid value");
							replay->log_packets = REPLAY_PACKETS_NONE;
						}
					}
					else if(!strcmp("events",word)) {
						word = strtok(NULL," \n");
						while((word) || (replay->log_events < 255)) {
							if(!strcmp("all",word)) {
								replay->log_events = REPLAY_EVENTS_ALL;
							}
							else if(!strcmp("adj-change",word)) {
								replay->log_events = replay->log_events + REPLAY_EVENTS_ADJ;
							}
							else if(!strcmp("if-change",word)) {
								replay->log_events = replay->log_events + REPLAY_EVENTS_IF;
							}
							else if(!strcmp("spf",word)) {
								replay->log_events = replay->log_events + REPLAY_EVENTS_SPF;
							}
							else if(!strcmp("auth",word)) {
								replay->log_events = replay->log_events + REPLAY_EVENTS_AUTH;
							}
							word = strtok(NULL," \n");
						}
						if(replay->log_events>255) {
							replay_error("config error: log block - event logging set to invalid value");
							replay->log_events = REPLAY_EVENTS_NONE;
						}
					}
					else if(!strcmp("lsdb-history",word)) {
						word = strtok(NULL," \n");
						if(word) {
							replay->lsdb_history = word[0] - '0';
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
					// load interface config
					break;

				}

			}

		}
	}

	if(!replay->errors) {
		replay->errors = fopen("errors.log","a");
	}
	if(!replay->packets) {
		replay->packets = replay->errors;
	}
	if(!replay->events) {
		replay->events = replay->errors;
	}
	if(!replay->lsdb) {
		replay->lsdb = fopen("lsdb.replay","a");
	}

	fclose(config);
}


void start_listening() {

	fd_set sockets_in, sockets_out, sockets_err;
	struct timeval tv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	sockets_in = ospf0->ospf_sockets_in;
	sockets_out = ospf0->ospf_sockets_out;
	sockets_err = ospf0->ospf_sockets_err;

	select(ospf0->max_socket+1,&sockets_in,&sockets_out,&sockets_err,&tv);

}
