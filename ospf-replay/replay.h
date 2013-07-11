/*
 * replay.h
 *
 *  Created on: May 19, 2013
 *      Author: nathan
 */

#ifndef REPLAY_H_
#define REPLAY_H_

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
#include <math.h>
#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "utility.h"
#include "lsa.h"

struct ospf {

	// Interfaces running OSPF
	struct replay_list *iflist;
	int ifcount;

	// Router ID
	struct in_addr router_id;

	// link state database
	struct ospf_lsdb *lsdb;

	// Routing table (outcome of SPF)
	struct ospf_route *route_table;

	// OSPF local prefix list
	struct replay_list *pflist;
	int pfxcount;
	int active_pfxcount;

	// OSPF neighbors
	struct replay_list *nbrlist;
	int nbrcount;

	// OSPF events awaiting execution
	struct replay_nlist *eventlist;

	// OSPF protocol sockets - listening set
	fd_set ospf_sockets_in;

	// OSPF protocol sockets - sending set
	fd_set ospf_sockets_out;

	// OSPF protocol sockets - errors set
	fd_set ospf_sockets_err;

	// keeps track of the socket with the highest ID number as needed by select()
	int max_socket;

	// global timers
	u_int32_t hello_interval, dead_interval, transmit_delay, retransmit_interval;

	// global cost setting
	u_int16_t cost;

	// passive interface default
	u_int8_t passif;

	// refence bandwidth
	unsigned long ref_bandwdith;

	// ospf options
	u_int8_t options;

	// ospf router priority
	u_int8_t priority;

	// ospf process started
	int started;

};

#define OSPF_DEFAULT_OPTIONS 0
#define OSPF_DEFAULT_PASSIF 0
#define OSPF_DEFAULT_HELLO 10
#define OSPF_DEFAULT_DEAD 40
#define OSPF_DEFAULT_TRANSMITDELAY 0
#define OSPF_DEFAULT_RETRANSMIT 0
#define OSPF_DEFAULT_COST 10000U
#define OSPF_DEFAULT_PRIORITY 1

#define OSPF_DEFAULT_REFERENCE_BANDWIDTH 100UL

#define OSPF_LSA_TYPES 9

struct ospf_lsdb {
	struct replay_nlist *lsa_list[OSPF_LSA_TYPES];
	unsigned long count;
	unsigned int checksum;
	struct ospf_lsa *this_rtr;
};

struct route_table {
	struct replay_list *route_list;
	struct route_array *by_mask[32];
	u_int32_t count;
};

struct route_array {
	struct route_entry *(*route_list)[];
};

struct route_entry {

	u_char src_type;
	struct in_addr net;
	struct in_addr mask;
	u_int8_t admin_dist;
	struct in_addr via;
	struct ifreq *iface;
	struct timeval tv;

};

#define OSPF_MULTICAST_ALLROUTERS "224.0.0.5"
#define OSPF_VERSION 0x02

#define TRUE 1
#define FALSE 0

#define WHITESPACE " \n\t"

#define OSPF_WAIT_FOR_DR 10

#define CHECK_BIT(var,pos) ((var) & (1<<((int)log2(pos))))

extern struct replay_config *replay0;
extern struct ospf *ospf0;

#endif /* REPLAY_H_ */
