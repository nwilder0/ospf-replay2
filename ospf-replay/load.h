/*
 * load.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#ifndef LOAD_H_
#define LOAD_H_

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
#include "utility.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "replay.h"

#define REPLAY_CONFIG_LOGGING 	1
#define REPLAY_CONFIG_ROUTER 	2
#define REPLAY_CONFIG_IF 		3

#define REPLAY_EVENTS_NONE	0x00
#define REPLAY_EVENTS_ALL	0xFF
#define REPLAY_EVENTS_ADJ	0x01
#define REPLAY_EVENTS_IF	0x02
#define REPLAY_EVENTS_SPF	0x04
#define REPLAY_EVENTS_AUTH	0x08

#define REPLAY_PACKETS_NONE		0x00
#define REPLAY_PACKETS_HELLO	0x01
#define REPLAY_PACKETS_DBDESC	0X02
#define REPLAY_PACKETS_LSR		0x04
#define REPLAY_PACKETS_LSU		0x08
#define REPLAY_PACKETS_LSACK	0x10
#define REPLAY_PACKETS_ALL		0xFF

#define REPLAY_LSDB_CURRENTONLY	0x00

struct replay_config {

	FILE *errors,*events,*packets,*lsdb;

	u_int8_t log_packets;
	u_int8_t log_events;
	u_int8_t lsdb_history;
	struct replay_list *iflist;

};

void load_defaults();
void load_config(const char*);


#endif /* LOAD_H_ */
