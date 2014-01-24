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
	u_int8_t virtual;
	FILE *replay, *record;
};

struct ospf_interface {
	struct replay_interface *iface;
	u_int32_t area_id;
	struct replay_list *pflist;
	struct replay_list *nbrlist;
	// list ospf_lsa objects replayed from this interface
	struct replay_nlist *lsalist;
	int ospf_socket;
	u_int16_t metric;
	struct in_addr bdr;
	struct in_addr dr;
	u_int8_t priority;
	u_int16_t auth_type;
	unsigned char auth_data[8];

};

#define OSPF_AUTHTYPE_NONE 0

#define REPLAY_VFACE_DUPLEX DUPLEX_FULL
#define REPLAY_VFACE_SPEED SPEED_1000
#define REPLAY_VFACE_MTU 1500
#define REPLAY_VFACE_INDEX 0
#define REPLAY_VFACE_FLAGS 0

struct replay_list* load_interfaces();
struct ospf_interface* add_interface(struct replay_interface*,u_int32_t);
void remove_interface(struct ospf_interface*);
struct replay_interface* find_interface(u_int32_t, u_int32_t);
struct replay_interface* find_interface_by_name(char*);
struct ospf_interface* find_oiface(struct replay_interface*);
struct replay_interface* new_viface(char*);
struct ospf_interface* iface_up(struct replay_interface*);
u_int16_t get_if_metric(struct ospf_interface*);
struct ospf_interface* find_oiface_by_socket(int);
void toggle_stub(struct ospf_interface*);
void refresh_virtuals();
void refresh_virtual(struct ospf_interface *);
void load_lsalist(struct ospf_interface *);

void system_if_up();
void system_if_down();


#endif /* INTERFACE_H_ */
