/*
 * prefix.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */
#ifndef PREFIX_H_
#define PREFIX_H_

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
#include "utility.h"
#include "replay.h"

struct ospf_prefix {
	struct in_addr network, mask;
	struct ifreq *iface;
	struct ospf_interface *ospf_if;
};

void add_prefix(char*,u_int32_t);
void remove_prefix(struct ospf_prefix*);

#endif /* PREFIX_H_ */