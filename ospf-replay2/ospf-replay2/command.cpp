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


int command(string str) {



}

int process_command(char *buffer) {

	int run_replay=1;
	char *word[10];

	word[0] = strtok(buffer,WHITESPACE);

	if(!strcmp("quit",word[0])) {
		run_replay=0;

	} else if(!strcmp("exit",word[0])) {
		replay0->cmd_state = REPLAY_CONFIG_NONE;
	} else if(!strcmp("logging",word[0])) {
		replay0->cmd_state = REPLAY_CONFIG_LOGGING;
	} else if(!strcmp("router",word[0])) {
		replay0->cmd_state = REPLAY_CONFIG_ROUTER;
	} else if(!strcmp("interface",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		if(word[1]) {
			replay0->cmd_state = REPLAY_CONFIG_IF;
			replay0->cmd_ifname = word[1];
			replay0->cmd_iface = find_interface_by_name(replay0->cmd_ifname);
			if(!replay0->cmd_iface) {
				replay0->cmd_iface = add_viface(replay0->cmd_ifname);
			}
		} else {
			replay_error("config error: interface command with no interface name");
		}

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
			}
		}
	} else if(!strcmp("read",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		word[2] = strtok(NULL,WHITESPACE);
		if(word[1]) {
			if(!strcmp("lsdb",word[1])) {
				read_lsdb(word[2]);
				printf("LSDB read from disk\n");
			}
		}
	} else if(!strcmp("remove",word[0])) {
		word[1] = strtok(NULL,WHITESPACE);
		word[2] = strtok(NULL,WHITESPACE);

		if(word[1]&&word[2]) {
			if(!strcmp("interface",word[1])) {
				struct replay_interface *iface = find_interface_by_name(word[2]);
				if(iface && iface->virtual) {
					remove_viface(iface);
				} else {
					replay_error("command error: unable to find a virtual interface to remove");
				}
			}
		}

	} else {
		switch(replay0->cmd_state) {
		case REPLAY_CONFIG_NONE:
			break;
		case REPLAY_CONFIG_LOGGING:
			command_logging(buffer);
			break;
		case REPLAY_CONFIG_ROUTER:
			command_router(buffer);
			break;
		case REPLAY_CONFIG_IF:
			command_if(buffer);
			break;
		}
	}

	print_prompt();

	return run_replay;
}

void command_logging(char *buffer) {

	char *word[10];
	char mesg[256]="";

	word[0] = strtok(buffer,WHITESPACE);
	word[1] = strtok(NULL,WHITESPACE);

	if(!strcmp("file",word[0])) {
		word[2] = strtok(NULL,WHITESPACE);
		if(word[1]&&word[2]) {
			FILE *log;
			if(!strcmp("all",word[1])) {
				log = replay0->packets = replay0->events = replay0->errors = fopen(word[2],"a");
			}
			else if(!strcmp("events",word[1])) {
				log = replay0->events = fopen(word[2],"a");
			}
			else if(!strcmp("packets",word[1])) {
				log = replay0->packets = fopen(word[2],"a");
			}
			else if(!strcmp("errors",word[1])) {
				log = replay0->errors = fopen(word[2],"a");
			}
			else if(!strcmp("lsdb",word[1])) {
				log = replay0->lsdb = fopen(word[2],"a+");
			}
			if(!log) {
				strcat(mesg,"file error: unable to open/create log file - ");
				strcat(mesg,word[2]);
				replay_error(mesg);
				mesg[0]='\0';
			}
		}
		else {
			replay_error("config error: logging - missing file type (all/events/packets/errors/lsdb) or filename");
		}
	}
	else if(!strcmp("packets",word[0])) {
		while((word[1]) && (replay0->log_packets < REPLAY_PACKETS_ALL)) {
			if(!strcmp("all",word[1])) {
				replay0->log_packets = REPLAY_PACKETS_ALL;
			}
			else if(!strcmp("hello",word[1])) {
				replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_HELLO;
			}
			else if(!strcmp("dbdesc",word[1])) {
				replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_DBDESC;
			}
			else if(!strcmp("lsr",word[1])) {
				replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSR;
			}
			else if(!strcmp("lsu",word[1])) {
				replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSU;
			}
			else if(!strcmp("lsack",word[1])) {
				replay0->log_packets = replay0->log_packets + REPLAY_PACKETS_LSACK;
			}
			word[1] = strtok(NULL,WHITESPACE);
		}
		if(replay0->log_packets>255) {
			replay_error("config error: log block - packet logging set to invalid value");
			replay0->log_packets = REPLAY_PACKETS_NONE;
		}
	}
	else if(!strcmp("events",word[0])) {
		while((word[1]) && (replay0->log_events < 255)) {
			if(!strcmp("all",word[1])) {
				replay0->log_events = REPLAY_EVENTS_ALL;
			}
			else if(!strcmp("adj-change",word[1])) {
				replay0->log_events = replay0->log_events + REPLAY_EVENTS_ADJ;
			}
			else if(!strcmp("if-change",word[1])) {
				replay0->log_events = replay0->log_events + REPLAY_EVENTS_IF;
			}
			else if(!strcmp("spf",word[1])) {
				replay0->log_events = replay0->log_events + REPLAY_EVENTS_SPF;
			}
			else if(!strcmp("auth",word[1])) {
				replay0->log_events = replay0->log_events + REPLAY_EVENTS_AUTH;
			}
			word[1] = strtok(NULL,WHITESPACE);
		}
		if(replay0->log_events>255) {
			replay_error("config error: log block - event logging set to invalid value");
			replay0->log_events = REPLAY_EVENTS_NONE;
		}
	}
	else if(!strcmp("lsdb-history",word[0])) {
		if(word[1]) {
			replay0->lsdb_history = word[1][0] - '0';
		}
		else {
			replay_error("config error: log block - no LSDB history number provided");
		}
	}

}

void command_router(char *buffer) {

	char *word[10];
	char mesg[256]="";

	word[0] = strtok(buffer,WHITESPACE);
	word[1] = strtok(NULL,WHITESPACE);
	if(!strcmp("router-id",word[0])) {
		if(word[1]) {
			inet_pton(AF_INET,word[1],&ospf0->router_id);
		} else {
			replay_error("config error: no id provided with router-id command");
		}
	}
	else if(!strcmp("passive-interface",word[0])) {
		ospf0->passif = 1;
	}
	else if(!strcmp("no",word[0])) {
		if(word[1]) {
			if(!strcmp("passive-interface",word[1])) {
				ospf0->passif = 0;
			}
		} else {
			replay_error("config error: invalid command following 'no'");
		}
	} else if(!strcmp("network",word[0])) {
		word[2] = strtok(NULL,WHITESPACE);
		word[3] = strtok(NULL,WHITESPACE);
		if(word[1] && !strcmp("area",word[2]) && word[3]) {
			add_prefix(word[1],atoi(word[3]));
		}
		else {
			// if the network statement is missing an IP/mask, area statement or area number
			// then record an error
			strcat(mesg,"config error: invalid format of line - ");
			strcat(mesg,buffer);
			replay_error(mesg);
			mesg[0]='\0';
		}
	} else if(!strcmp("cost",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->cost = OSPF_DEFAULT_COST;
				sprintf(mesg,"config: cost set to %u",ospf0->cost);
				replay_log(mesg);
			} else {
				u_int16_t value = atoi(word[1]);
				if(value > 0 && value < 65536) {
					ospf0->cost = value;
					sprintf(mesg,"config: cost set to %u",ospf0->cost);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for cost are 1-65535");
				}
			}
		} else {
			replay_error("config error: cost command with no cost value");
		}
	} else if(!strcmp("dead-interval",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->dead_interval = OSPF_DEFAULT_DEAD;
				sprintf(mesg,"config: dead interval set to %u",ospf0->dead_interval);
				replay_log(mesg);
			} else {
				u_int32_t value = atol(word[1]);
				if(value > 0 && value < 4294967296) {
					ospf0->dead_interval = value;
					sprintf(mesg,"config: dead interval set to %u",ospf0->dead_interval);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for dead interval are 1-4294967295");
				}
			}
		} else {
			replay_error("config error: dead-interval command with no value");
		}
	} else if(!strcmp("hello-interval",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->hello_interval = OSPF_DEFAULT_HELLO;
				sprintf(mesg,"config: hello interval set to %u",ospf0->hello_interval);
				replay_log(mesg);
			} else {
				u_int32_t value = atol(word[1]);
				if(value > 0 && value < 4294967296) {
					ospf0->hello_interval = value;
					sprintf(mesg,"config: hello interval set to %u",ospf0->hello_interval);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for hello interval are 1-4294967295");
				}
			}
		} else {
			replay_error("config error: hello-interval command with no value");
		}
	} else if(!strcmp("retransmit-interval",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->retransmit_interval = OSPF_DEFAULT_RETRANSMIT;
				sprintf(mesg,"config: retransmit interval set to %u",ospf0->retransmit_interval);
				replay_log(mesg);
			} else {
				u_int32_t value = atol(word[1]);
				if(value > 0 && value < 4294967296) {
					ospf0->retransmit_interval = value;
					sprintf(mesg,"config: retransmit interval set to %u",ospf0->retransmit_interval);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for retransmit interval are 1-4294967296");
				}
			}
		} else {
			replay_error("config error: retransmit-interval command with no value");
		}
	} else if(!strcmp("transmit-delay",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->transmit_delay = OSPF_DEFAULT_TRANSMITDELAY;
				sprintf(mesg,"config: transmit delay set to %u",ospf0->transmit_delay);
				replay_log(mesg);
			} else {
				u_int32_t value = atol(word[1]);
				if(value > 0 && value < 4294967296) {
					ospf0->transmit_delay = value;
					sprintf(mesg,"config: transmit delay set to %u",ospf0->transmit_delay);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for transmit delay are 1-4294967295");
				}
			}
		} else {
			replay_error("config error: transmit-delay command with no value");
		}
	} else if(!strcmp("reference-bandwidth",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->ref_bandwdith = OSPF_DEFAULT_REFERENCE_BANDWIDTH;
				sprintf(mesg,"config: reference bandwidth set to %lu",ospf0->ref_bandwdith);
				replay_log(mesg);
			} else {
				u_int32_t value = atol(word[1]);
				if(value > 0 && value < 4294967296) {
					ospf0->ref_bandwdith = value;
					sprintf(mesg,"config: reference bandwidth set to %lu",ospf0->ref_bandwdith);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for reference bandwidth are 1-4294967295");
				}
			}
		} else {
			replay_error("config error: reference-bandwidth command with no value");
		}
	} else if(!strcmp("options",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->options = OSPF_DEFAULT_OPTIONS;
				sprintf(mesg,"config: options set to %x",ospf0->options);
				replay_log(mesg);
			} else {
				u_int8_t value = (u_int8_t)strtoul(word[1],NULL,16);
				if(value > 0 && value < 256) {
					ospf0->options = value;
					sprintf(mesg,"config: options set to %x",ospf0->options);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for options are 0x00 to 0xFF");
				}
			}
		} else {
			replay_error("config error: options command with no value");
		}
	} else if(!strcmp("priority",word[0])) {
		if(word[1]) {
			if(!strcmp("default",word[1])) {
				ospf0->priority = OSPF_DEFAULT_PRIORITY;
				sprintf(mesg,"config: priority set to %u",ospf0->priority);
				replay_log(mesg);
			} else {
				u_int8_t value = atoi(word[1]);
				if(value > 0 && value < 256) {
					ospf0->priority = value;
					sprintf(mesg,"config: priority set to %u",ospf0->priority);
					replay_log(mesg);
				} else {
					replay_error("config error: valid values for priority are 1-255");
				}
			}
		} else {
			replay_error("config error: priority command with no value");
		}
	}

}

void command_if(char *buffer) {

	char *word[10];
	char mesg[256]="";

	word[0] = strtok(buffer,WHITESPACE);
	word[1] = strtok(NULL,WHITESPACE);
	// load the FILE object to record LSAs to
	if(!strcmp("record",word[0])) {
		if(word[1]) {
			replay0->cmd_iface->record = fopen(word[1],"a");
			if(!replay0->cmd_iface->record) {
				strcat(mesg,"file error: unable to open/create record log file - ");
				strcat(mesg,word[1]);
				replay_error(mesg);
				mesg[0]='\0';
			}
		} else {
			replay_error("config error: record command without filename");
		}
	}
	// load the FILE object to replay LSAs from
	else if(!strcmp("replay",word[0])) {
		if(word[1]) {
			replay0->cmd_iface->replay = fopen(word[1],"r");
			if(!replay0->cmd_iface->replay) {
				strcat(mesg,"file error: unable to open replay log file - ");
				strcat(mesg,word[1]);
				replay_error(mesg);
				mesg[0]='\0';
			} else {
				// if there is already an OSPF interface for this interface, then load the LSA list to it
				struct ospf_interface *oiface = find_oiface(replay0->cmd_iface);
				if(oiface) {
					load_lsalist(oiface);
				}
			}
		} else {
			replay_error("config error: record command without filename");
		}
	}
	// sets the IP for a virtual interface
	else if(!strcmp("ip",word[0])) {
		// set the IP if this is a virtual interface
		if(word[1]) {
			if(replay0->cmd_iface->virtual) {
				strcpy(word[2],strtok(word[1],"/"));
				strcpy(word[3],strtok(NULL,"/"));
				if(word[2]&&word[3]) {
					replay0->cmd_iface->mask.s_addr = bits2mask(atoi(word[3]));

					inet_pton(AF_INET,word[2],&replay0->cmd_iface->ip);
					iface_up(replay0->cmd_iface);
				} else {
					replay_error("config error: invalid format for ip command (ip X.X.X.X/Y)");
				}
			} else {
				replay_error("config error: ip command can only be used on virtual interfaces");
			}
		} else {
			replay_error("config error: ip command with no ip address / mask");
		}
	}
}

void print_prompt() {

	if(replay0->cmd_state == REPLAY_CONFIG_NONE) {
		printf(">");
	} else if(replay0->cmd_state == REPLAY_CONFIG_LOGGING) {
		printf("logging>");
	} else if(replay0->cmd_state == REPLAY_CONFIG_ROUTER) {
		printf("router>");
	} else if(replay0->cmd_state == REPLAY_CONFIG_IF) {
		printf("%s>",replay0->cmd_ifname);
	}

	fflush(stdout);
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

