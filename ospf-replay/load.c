/*
 * load.c
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "utility.h"
#include "replay.h"

void load_defaults() {

	replay_log("load_defaults: starting function\n");

	replay0->iflist = load_interfaces();

	replay_log("load_defaults: ospf0 numeric members to defaults");
	// initialize the OSPF process numeric members to their defaults
	ospf0->passif = OSPF_DEFAULT_PASSIF;
	ospf0->cost = OSPF_DEFAULT_COST;
	ospf0->dead_interval = OSPF_DEFAULT_DEAD;
	ospf0->hello_interval = OSPF_DEFAULT_HELLO;
	ospf0->retransmit_interval = OSPF_DEFAULT_RETRANSMIT;
	ospf0->transmit_delay = OSPF_DEFAULT_TRANSMITDELAY;
	ospf0->ref_bandwdith = OSPF_DEFAULT_REFERENCE_BANDWIDTH;
	ospf0->options = OSPF_DEFAULT_OPTIONS;
	ospf0->priority = OSPF_DEFAULT_PRIORITY;

	ospf0->lsdb = malloc(sizeof(*ospf0->lsdb));
	memset(ospf0->lsdb,0,sizeof(*ospf0->lsdb));

	replay_log("load_defaults: Clearing out the select() file/stream sets\n");
	// clear out the select() file/stream descriptor sets
	FD_ZERO(&ospf0->ospf_sockets_err);
	FD_ZERO(&ospf0->ospf_sockets_in);
	FD_ZERO(&ospf0->ospf_sockets_out);

	replay_log("load_defaults: Adding stdin to select() input set\n");
	// add stdin to the select() input set, this allows us to have a command line which accepts commands while we are looking for packets
	FD_SET(0, &ospf0->ospf_sockets_in);

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
	char *word, *word2, *word3, *ifname, *logfilename;
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
				word = strtok(line,WHITESPACE);
				if(!strcmp("logging",word)) {
					config_block = REPLAY_CONFIG_LOGGING;
				}
				else if(!strcmp("router",word)) {
					config_block = REPLAY_CONFIG_ROUTER;
				}
				else if(!strcmp("interface",word)) {
					ifname = strtok(NULL,WHITESPACE);
					if(ifname) {
						config_block = REPLAY_CONFIG_IF;
					}
				}
				else if(config_block) {

					switch(config_block) {

					case REPLAY_CONFIG_LOGGING:
						if(!strcmp("file",word)) {
							word = strtok(NULL,WHITESPACE);
							if(word) {
								logfilename = strtok(NULL,WHITESPACE);
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
							word = strtok(NULL,WHITESPACE);
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
								word = strtok(NULL,WHITESPACE);
							}
							if(replay0->log_packets>255) {
								replay_error("config error: log block - packet logging set to invalid value");
								replay0->log_packets = REPLAY_PACKETS_NONE;
							}
						}
						else if(!strcmp("events",word)) {
							word = strtok(NULL,WHITESPACE);
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
								word = strtok(NULL,WHITESPACE);
							}
							if(replay0->log_events>255) {
								replay_error("config error: log block - event logging set to invalid value");
								replay0->log_events = REPLAY_EVENTS_NONE;
							}
						}
						else if(!strcmp("lsdb-history",word)) {
							word = strtok(NULL,WHITESPACE);
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
						if(!strcmp("router-id",word)) {
							word = strtok(NULL,WHITESPACE);
							if(word) {
								inet_pton(AF_INET,word,&ospf0->router_id);
							}
						}
						else if(!strcmp("passive-interface",word)) {
							ospf0->passif = 1;
						}
						else if(!strcmp("no",word)) {
							word = strtok(NULL,WHITESPACE);
							if(word) {
								if(!strcmp("passive-interface",word)) {
									ospf0->passif = 0;
								}
							}
						}
						else if(!strcmp("network",word)) {
							word = strtok(NULL,WHITESPACE);
							word2 = strtok(NULL,WHITESPACE);
							word3 = strtok(NULL,WHITESPACE);
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

void unload_replay() {
	struct ospf_event *tmp_event;
	struct ospf_interface *tmp_if;
	struct ospf_neighbor *tmp_nbr;
	struct ospf_prefix *tmp_pfx;
	struct ospf_lsa *tmp_lsa;

	while(ospf0->eventlist) {
		tmp_event = (struct ospf_event *)ospf0->eventlist->object;
		if(tmp_event) {
			remove_event(tmp_event);
		} else {
			ospf0->eventlist = remove_from_nlist(ospf0->eventlist,ospf0->eventlist);
		}

	}

	while(ospf0->nbrlist) {
		tmp_nbr = (struct ospf_neighbor *)ospf0->nbrlist->object;

		if(tmp_nbr) {
			remove_neighbor(tmp_nbr);
		} else {
			ospf0->nbrlist = remove_from_list(ospf0->nbrlist,ospf0->nbrlist);
		}
	}

	while(ospf0->iflist) {
		tmp_if = (struct ospf_interface *)ospf0->iflist->object;

		if(tmp_if) {
			remove_interface(tmp_if);
		} else {
			ospf0->iflist = remove_from_list(ospf0->iflist,ospf0->iflist);
		}
	}


	while(ospf0->pflist) {
		tmp_pfx = (struct ospf_prefix *)ospf0->pflist->object;

		if(tmp_pfx) {
			remove_prefix(tmp_pfx);
		} else {
			ospf0->pflist = remove_from_list(ospf0->pflist,ospf0->pflist);
		}
	}

	//remove lsdb
	if(ospf0->lsdb) {
		int i=0;
		for(i=0;i<OSPF_LSA_TYPES;i++) {
			while(ospf0->lsdb->lsa_list[i]) {
				tmp_lsa = (struct ospf_lsa *) ospf0->lsdb->lsa_list[i]->object;
				if(tmp_lsa) {
					remove_lsa(tmp_lsa);
				} else {
					ospf0->lsdb->lsa_list[i] = remove_from_nlist(ospf0->lsdb->lsa_list[i],ospf0->lsdb->lsa_list[i]);
				}
			}
		}
		ospf0->lsdb->this_rtr = NULL;
		free(ospf0->lsdb);
	}

	//if route table ever added it will need to be freed

	delete_list(replay0->iflist);
	fclose(replay0->errors);
	fclose(replay0->events);
	fclose(replay0->lsdb);
	if(replay0->errors != replay0->packets) {
		fclose(replay0->packets);
	}
	free(ospf0);
	free(replay0);
}

