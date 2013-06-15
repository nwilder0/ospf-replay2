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
#include <linux/sockios.h>
#include <linux/ethtool.h>
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

struct replay_interface {
	struct ifreq *ifr;
	char name[256];
	struct in_addr ip;
	struct in_addr mask;
	long speed;
	long mtu;
	int index;
	int duplex;
	int flags;
};

struct ospf_interface {
	struct replay_interface *iface;
	u_int32_t area_id;
	struct replay_list *pflist;
	struct replay_list *nbrlist;
	int ospf_socket;
	u_int16_t metric;
};

struct replay_list* load_interfaces();
struct ospf_interface* add_interface(struct replay_interface*,u_int32_t);
void remove_interface(struct ospf_interface*);
struct replay_interface* find_interface(u_int32_t, u_int32_t);
u_int16_t get_if_metric(struct ospf_interface*);

sytem_if_up();
system_if_down();


#endif /* INTERFACE_H_ */
