/*
 * replay.h
 *
 *  Created on: May 19, 2013
 *      Author: nathan
 */

#ifndef REPLAY_H_
#define REPLAY_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


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

};

struct ospf {

	// Interfaces running OSPF
	struct ospf_interface *iflist;

	// Router ID
	struct in_addr router_id;

	// link state database
	struct ospf_lsdb *lsdb;

	// OSPF local prefix list
	struct ospf_prefix *pflist;

	// OSPF neighbors
	struct ospf_neighbor *nbrlist;

	// OSPF protocol sockets - listening set
	fd_set ospf_sockets_in;

	// OSPF protocol sockets - sending set
	fd_set ospf_sockets_out;

	// OSPF protocol sockets - errors set
	fd_set ospf_sockets_err;

	// keeps track of the socket with the highest ID number as needed by select()
	int max_socket;

	// global timers
	int hello_interval, dead_interval, transmit_delay, retransmit_interval;

	// global cost setting
	u_int8_t cost;

};

struct ospf_prefix {
	struct in_addr network, mask;
	struct ifreq *iface;

};

struct ospf_neighbor {

};

struct ospf_interface {


};

struct ospf_lsdb {


};

struct ospfhdr {

	u_int8_t version;
	u_int8_t mesg_type;
	u_int16_t packet_length;
	u_int32_t src_router;
	u_int32_t area_id;
	u_int16_t checksum;
	u_int16_t auth_type;
	unsigned char auth_data[8];

};

#define OSPF_MESG_HELLO 	0x01
#define OSPF_MESG_DBDESC	0x02
#define OSPF_MESG_LSR		0x03
#define OSPF_MESG_LSU		0x04
#define OSPF_MESG_LSACK		0x05

/* OSPF Hello body format. */
struct ospf_hello
{
  struct in_addr network_mask;
  u_short hello_interval;
  u_char options;
  u_char priority;
  u_int32_t dead_interval;
  struct in_addr dr;
  struct in_addr bdr;
  struct in_addr neighbors[1];
};

#define OSPFHDR_LEN 24

#endif /* REPLAY_H_ */
