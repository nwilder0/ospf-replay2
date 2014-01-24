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

		// read until the end of the file
		while(!feof(config)) {

			// if a line is successfully read
			if(fgets(line,sizeof(line),config)) {

				// process the line as a command
				process_command(line);

			}
		}
		// close the config file
		fclose(config);
		replay_log("load_config: config file closed");
	}
	// set default files for any logging files not specified in the config file
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
		replay0->lsdb = fopen("lsdb.replay","a+");
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

	while(replay0->iflist) {
		struct replay_interface *tmp_rif = (struct replay_interface *)replay0->iflist->object;
		if(tmp_rif) {
			if(tmp_rif->record) {
				fclose(tmp_rif->record);
			}
			if(tmp_rif->replay) {
				fclose(tmp_rif->replay);
			}
		}
		replay0->iflist = remove_from_list(replay0->iflist,replay0->iflist);
	}

	if(replay0->errors != replay0->packets) {
		fclose(replay0->packets);
	}
	if(replay0->errors != replay0->events) {
		fclose(replay0->events);
	}
	fclose(replay0->errors);
	fclose(replay0->lsdb);
	free(ospf0);
	free(replay0);
}

