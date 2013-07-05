/*
 * interface.c
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

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
#include "prefix.h"
#include "utility.h"
#include "replay.h"

struct replay_list* load_interfaces() {

	struct replay_interface *iface;
	struct replay_list *new_item, *prev_item;
	struct ifreq *ifr;
	struct ifconf ifc;
	struct ethtool_cmd cmd;
	int s, rc, i;
	int numif;

	// find number of interfaces.
	memset(&ifc,0,sizeof(ifc));
	ifc.ifc_ifcu.ifcu_req = NULL;
	ifc.ifc_len = 0;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		replay_error("find_interface: unable to open socket");
	}

	if ((rc = ioctl(s, SIOCGIFCONF, &ifc)) < 0) {
		replay_error("find_interface: error on 1st ioctl");
	}

	numif = ifc.ifc_len / sizeof(struct ifreq);
	if ((ifr = malloc(ifc.ifc_len)) == NULL) {
		replay_error("find_interface: error on malloc of ifreq");
	}
	ifc.ifc_ifcu.ifcu_req = ifr;

	if ((rc = ioctl(s, SIOCGIFCONF, &ifc)) < 0) {
		replay_error("find_interface: error on 2nd ioctl");
	}

	prev_item = NULL;

	for(i = 0; i < numif; i++) {

		struct ifreq *r = &ifr[i];
		struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
		struct sockaddr_in *smask = (struct sockaddr_in *)&r->ifr_netmask;

		iface = (struct replay_interface *) malloc(sizeof(struct replay_interface));
		new_item = (struct replay_list *) malloc(sizeof(struct replay_list));

		iface->index = i;
		iface->ip.s_addr = sin->sin_addr.s_addr;
		iface->mask.s_addr = smask->sin_addr.s_addr;
		iface->mtu = r->ifr_mtu;
		strcpy(iface->name,r->ifr_name);

		new_item->next = prev_item;
		new_item->object = (struct replay_object *) iface;

		/* Interface flags. */
		if (ioctl(s, SIOCGIFFLAGS, ifr) == -1)
			iface->flags = 0;
		else
			iface->flags = ifr->ifr_flags;

		ifr->ifr_data = (void *)&cmd;
		cmd.cmd = ETHTOOL_GSET; /* "Get settings" */
		if (ioctl(s, SIOCETHTOOL, ifr) == -1) {
			/* Unknown */
			iface->speed = -1L;
			iface->duplex = DUPLEX_UNKNOWN;
		} else {
			iface->speed = ethtool_cmd_speed(&cmd);
		    iface->duplex = cmd.duplex;
		}
		prev_item = new_item;
	}

	close(s);

	return prev_item;
}

struct replay_interface* find_interface(uint32_t net_addr, uint32_t mask_addr) {

	struct replay_interface *search_if, *found;
	struct replay_list *search_item;
	uint32_t if_net;

	search_item = replay0->iflist;
	found = NULL;

	while(search_item) {
		if(search_item->object) {
			search_if = (struct replay_interface *)search_item->object;
			if_net = get_net(search_if->ip.s_addr,search_if->mask.s_addr);
			if (if_net == net_addr && mask_addr == search_if->mask.s_addr) {
				found = search_if;
			}
		}
		search_item = search_item->next;
	}
	return found;
}

struct ospf_interface* find_oiface_by_socket(int socket) {

	struct ospf_interface *search_if, *found;
	struct replay_list *search_item;

	search_item = ospf0->iflist;
	found = NULL;

	while(search_item) {
		if(search_item->object) {
			search_if = (struct ospf_interface *)search_item->object;
			if (search_if->ospf_socket == socket) {
				found = search_if;
			}
		}
		search_item = search_item->next;
	}
	return found;
}

struct ospf_interface* add_interface(struct replay_interface *iface, u_int32_t area) {

	struct ospf_interface *new_if, *tmp_if;
	struct replay_list *tmp_item;
	struct ip_mreq mreq;
	struct sockaddr_in *sin = (struct sockaddr_in *)&iface->ifr->ifr_addr;
	int ospf_socket;
	char ifname[256];
	int duplicate=FALSE;

	strcpy(ifname,iface->name);

	tmp_item = ospf0->iflist;
	if(tmp_item) {
		tmp_if = (struct ospf_interface*)tmp_item->object;
	}
	while(tmp_item && tmp_if) {
		if(!strcmp(ifname,tmp_if->iface->name)) {
			duplicate = TRUE;
			new_if = tmp_if;
			tmp_item = NULL;
			tmp_if = NULL;
		}
		else {
			tmp_item = tmp_item->next;
			if(!tmp_item)
				tmp_if = (struct ospf_interface*)tmp_item->object;
		}
	}

	if(!duplicate) {
		new_if = (struct ospf_interface *) malloc(sizeof(struct ospf_interface));

		new_if->area_id = area;
		new_if->iface = iface;
		new_if->pflist = NULL;
		new_if->nbrlist = NULL;
		new_if->metric = ospf0->ref_bandwdith / iface->speed;
		inet_pton(AF_INET,"0.0.0.0",&new_if->bdr);
		inet_pton(AF_INET,"0.0.0.0",&new_if->dr);
		new_if->auth_type = OSPF_AUTHTYPE_NONE;
		bzero((char *) &new_if->auth_data, sizeof(new_if->auth_data));

		ospf0->iflist = add_to_list(ospf0->iflist,(struct replay_object *)new_if);

		ospf_socket = socket(AF_INET,SOCK_RAW,89);

		if(ospf_socket<0)
			replay_error("add_interface: Error opening socket");
		else
			replay_log("add_interface: socket opened");

		if (setsockopt(ospf_socket, SOL_SOCKET, SO_BINDTODEVICE, &iface->ifr, sizeof(iface->ifr)) < 0)
			replay_error("add_interface: ERROR on interface binding");
		else
			replay_log("add_interface: socket bound to interface");

		mreq.imr_interface.s_addr = sin->sin_addr.s_addr;
		inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&mreq.imr_multiaddr);

		if(setsockopt(ospf_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))<0)
			replay_error("add_interface: ERROR on multicast join");
		else
			replay_log("add_interface: multicast group added");

		FD_SET(ospf_socket, &ospf0->ospf_sockets_in);
		FD_SET(ospf_socket, &ospf0->ospf_sockets_out);
		FD_SET(ospf_socket, &ospf0->ospf_sockets_err);

		ospf0->ifcount++;
		add_event((struct replay_object *)new_if,OSPF_EVENT_HELLO_BROADCAST);
		add_event((struct replay_object *)new_if,OSPF_EVENT_NO_DR);
	}
	return new_if;

}

void remove_interface(struct ospf_interface *ospf_if) {
	struct ospf_prefix *tmp_pfx;
	struct replay_list *tmp_item,*next;
	struct ospf_interface *tmp_if;
	struct ospf_neighbor *tmp_nbr;

	if(ospf_if) {
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_in);
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_out);
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_err);

		remove_object_events((struct replay_object *)ospf_if);

		tmp_item = ospf_if->pflist;
		while(tmp_item) {
			next = tmp_item->next;
			tmp_pfx = (struct ospf_prefix *)tmp_item->object;
			tmp_pfx->ospf_if = NULL;
			free(tmp_item);
			tmp_item = next;
		}
		tmp_item = ospf_if->nbrlist;
		while(tmp_item) {
			next = tmp_item->next;
			tmp_nbr = (struct ospf_neighbor *)tmp_item->object;
			remove_neighbor(tmp_nbr);
			free(tmp_item);
			tmp_item = next;
		}

		tmp_item = ospf0->iflist;
		while(tmp_item) {
			tmp_if = (struct ospf_interface *)tmp_item->object;
			if(tmp_if==ospf_if) {
				ospf0->iflist = remove_from_list(ospf0->iflist,tmp_item);
			}
		}
		if(ospf_if->ospf_socket)
			close(ospf_if->ospf_socket);

		free(ospf_if);
		ospf0->ifcount--;
	}
	else {
		tmp_item = ospf0->iflist;
		while(tmp_item) {
			next = tmp_item;
			tmp_if = (struct ospf_interface *)tmp_item->object;
			remove_interface(tmp_if);
			tmp_item = next;
		}
		ospf0->iflist = NULL;
	}

}

u_int16_t get_if_metric(struct ospf_interface *ospf_if) {
	if(ospf_if) {
		return ospf_if->metric;
	}
	else {
		return 1;
	}
}
