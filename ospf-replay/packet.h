/*
 * packet.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */
#ifndef PACKET_H_
#define PACKET_H_

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
#include "prefix.h"
#include "utility.h"
#include "replay.h"

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

void process_packet(int);

#endif /* PACKET_H_ */
