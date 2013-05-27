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

#endif /* REPLAY_H_ */

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
