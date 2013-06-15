/*
 * neighbor.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#ifndef NEIGHBOR_H_
#define NEIGHBOR_H_

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
#include "prefix.h"
#include "packet.h"
#include "utility.h"
#include "replay.h"

struct ospf_neighbor {
	struct in_addr router_id, ip;
	struct ospf_interface *ospf_if;
	u_int8_t state;
};

#define OSPF_NBRSTATE_DOWN 		0
#define OSPF_NBRSTATE_ATTEMPT 	1
#define OSPF_NBRSTATE_INIT 		2
#define OSPF_NBRSTATE_2WAY 		3
#define OSPF_NBRSTATE_EXSTART 	4
#define OSPF_NBRSTATE_EXCHANGE 	5
#define OSPF_NBRSTATE_LOADING 	6
#define OSPF_NBRSTATE_FULL 		7


struct ospf_neighbor* add_neighbor(u_int32_t,u_int32_t,struct ospf_interface*);
void remove_neighbor(struct ospf_neighbor*);

#endif /* NEIGHBOR_H_ */
