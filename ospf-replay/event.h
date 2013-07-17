/*
 * event.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#ifndef EVENT_H_
#define EVENT_H_

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
#include "prefix.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "utility.h"
#include "replay.h"

struct ospf_event {
	struct replay_object *object;
	struct timeval tv;
	u_int8_t type;
};

#define OSPF_EVENT_HELLO_BROADCAST 0
#define OSPF_EVENT_LSA_AGING 1
#define OSPF_EVENT_NBR_DEAD 2
#define OSPF_EVENT_NO_DR 3
#define OSPF_EVENT_DBDESC_RETX 4
#define OSPF_EVENT_LSR_RETX 5
#define OSPF_EVENT_LSU_ACK 6

void check_events();
void do_event(struct replay_nlist*);
void add_event(struct replay_object*,u_int8_t);
void remove_event(struct ospf_event*);
void remove_object_events(struct replay_object*);
struct ospf_event* find_event(struct replay_object*,u_int8_t);
void find_and_remove_event(struct replay_object*,u_int8_t);



#endif /* EVENT_H_ */
