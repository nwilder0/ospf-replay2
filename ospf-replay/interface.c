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


struct ifreq* find_interface(uint32_t net_addr, uint32_t mask_addr) {

	struct ifreq *ifr, *found;
	struct ifconf ifc;
	int s, rc, i;
	int numif;
	uint32_t if_net;

	found = NULL;

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

	close(s);

	for(i = 0; i < numif; i++) {

		struct ifreq *r = &ifr[i];
		struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
		if_net = get_net(sin->sin_addr.s_addr,mask_addr);
		if (if_net == net_addr) {
			found = r;
		}
	}
	return found;
}

struct ospf_interface* add_interface(struct ifreq *iface, u_int32_t area) {

	struct ospf_interface *new_if, *tmp_if;
	struct replay_list *tmp_item, *new_item;
	struct ip_mreq mreq;
	int ospf_socket;
	struct sockaddr_in *sin = (struct sockaddr_in *)&iface->ifr_addr;
	char ifname[16];
	int duplicate=FALSE;

	strcpy(ifname,iface->ifr_ifrn.ifrn_name);

	tmp_item = ospf0->iflist;
	if(tmp_item) {
		tmp_if = (struct ospf_interface*)tmp_item->object;
	}
	while(tmp_item && tmp_if) {
		if(!strcmp(ifname,tmp_if->iface->ifr_ifrn.ifrn_name)) {
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
		new_item = (struct replay_list *) malloc(sizeof(struct replay_list));

		new_if->area_id = area;
		new_if->iface = iface;
		new_if->pflist = NULL;
		new_if->nbrlist = NULL;

		new_item->next = NULL;
		new_item->object = (struct replay_object *)new_if;

		ospf0->iflist = add_to_list(ospf0->iflist,new_item);

		ospf_socket = socket(AF_INET,SOCK_RAW,89);

		if(ospf_socket<0)
			replay_error("add_interface: Error opening socket");
		else
			replay_log("add_interface: socket opened");

		if (setsockopt(ospf_socket, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface)) < 0)
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
	}
	return new_if;

}

void remove_interface(struct ospf_interface *ospf_if) {
	struct ospf_prefix *tmp_pfx;
	struct replay_list *tmp_item,*next;
	struct ospf_interface *tmp_if;
	struct ospf_neighbor *tmp_nbr;
	struct ospf_event *tmp_event;

	if(ospf_if) {
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_in);
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_out);
		FD_CLR(ospf_if->ospf_socket,&ospf0->ospf_sockets_err);

		tmp_item = ospf0->eventlist;
		while(tmp_item) {
			tmp_event = (struct ospf_event *)tmp_item->object;
			if(tmp_event->object == ospf_if) {
				ospf0->eventlist = remove_from_list(ospf0->eventlist,tmp_item);
				free(tmp_event);
			}
		}
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
