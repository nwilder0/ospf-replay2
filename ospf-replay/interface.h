/*
 * interface.h
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

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
#include "utility.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "replay.h"

struct ospf_interface {
	struct ifreq *iface;
	u_int32_t area_id;
	struct replay_list *pflist;
	struct replay_list *nbrlist;
	int ospf_socket;
};

struct ospf_interface* add_interface(struct ifreq*,u_int32_t);
void remove_interface(struct ospf_interface*);
struct ifreq* find_interface(u_int32_t, u_int32_t);


#endif /* INTERFACE_H_ */
