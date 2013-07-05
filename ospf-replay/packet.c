/*
 * packet.c
 *
 *  Created on: Jun 8, 2013
 *      Author: nathan
 */

#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "packet.h"
#include "prefix.h"
#include "utility.h"
#include "replay.h"

void process_packet(int socket) {

	unsigned char *buffer;
	int size = 0;
	struct sockaddr_in recv_addr;
	struct iphdr *ipheader;
	struct ospfhdr *ospfheader;
	unsigned int iphdr_len, recv_addr_len;
	struct ospf_interface *oiface;

	buffer = (unsigned char *)malloc(sizeof(char)*REPLAY_PACKET_BUFFER);
	bzero((char *) &recv_addr, sizeof(recv_addr));
	recv_addr_len = sizeof(recv_addr);

	size = recvfrom(socket, buffer, REPLAY_PACKET_BUFFER, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
	if(size>0) {
		oiface = find_oiface_by_socket(socket);
		ipheader = (struct iphdr *)buffer;
		iphdr_len = ipheader->ihl*4;
		ospfheader = (struct ospfhdr *)(buffer+iphdr_len);
		switch(ospfheader->mesg_type) {

		case OSPF_MESG_HELLO:
			process_hello(buffer+iphdr_len,ipheader->saddr,ipheader->daddr,((unsigned int)ntohs(ospfheader->packet_length)),oiface);
			break;
		}
	} else {
		//error reading in packet
	}
	free(buffer);

}

void send_hello(struct ospf_interface *ospf_if,struct ospf_neighbor *neighbor) {

	struct in_addr src_addr, remote_addr;
	void *packet;
	struct ospf_hello *hello;
	struct replay_list *tmp_item;
	struct ospf_neighbor *tmp_neighbor;
	int i,nbrcount,size;

	src_addr.s_addr = ospf_if->iface->ip.s_addr;

	if(neighbor) remote_addr.s_addr = neighbor->ip.s_addr;
	else inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&remote_addr);

	nbrcount = ospf0->nbrcount;
	size = sizeof(sizeof(struct ospfhdr)+sizeof(struct ospf_hello)+((nbrcount-1)*sizeof(u_int32_t)));
	packet = malloc(size);
	hello = (struct ospf_hello *)(packet + sizeof(struct ospfhdr));

	hello->bdr.s_addr = ospf_if->bdr.s_addr;
	hello->dead_interval = ospf0->dead_interval;
	hello->dr.s_addr = ospf_if->dr.s_addr;
	hello->hello_interval = ospf0->hello_interval;
	hello->network_mask.s_addr = ospf_if->iface->mask.s_addr;
	hello->options = ospf0->options;
	hello->priority = ospf0->priority;

	tmp_item = ospf0->nbrlist;
	for(i=0;i<nbrcount;i++) {
		if(tmp_item) {
			tmp_neighbor = (struct ospf_neighbor *)tmp_item->object;
			if (tmp_neighbor) hello->neighbors[i].s_addr = tmp_neighbor->router_id.s_addr;
			tmp_item = tmp_item->next;
		}
	}

	build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_HELLO,packet,size,ospf_if);

	send_packet(ospf_if,packet,remote_addr.s_addr,size);

}

void build_ospf_packet(u_int32_t src_addr,u_int32_t remote_addr,u_int8_t mesg_type,void *packet,int size,struct ospf_interface *ospf_if) {
	struct ospfhdr *header;

	header = (struct ospfhdr *)packet;
	header->area_id = ospf_if->area_id;
	header->mesg_type = mesg_type;
	header->packet_length = htons(size - sizeof(struct iphdr));
	header->src_router = ospf0->router_id.s_addr;
	header->version = OSPF_VERSION;
	header->auth_type = ospf_if->auth_type;
	memcpy(header->auth_data,ospf_if->auth_data,sizeof(header->auth_data));
	header->checksum = in_cksum((void *)header,ntohs(header->packet_length));

}


void send_packet(struct ospf_interface *ospf_if,void *packet,u_int32_t remote_addr,int size) {
	int error;
	struct sockaddr_in dest_addr;

	bzero((char *) &dest_addr, sizeof(dest_addr));

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = remote_addr;

	error = sendto(ospf_if->ospf_socket,packet,size,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
	if(error<1) {

	}
}

void process_hello(void *packet, u_int32_t from, u_int32_t to, unsigned int size, struct ospf_interface *oiface) {
	struct ospfhdr *ospfheader;
	struct ospf_hello *hello;
	struct ospf_neighbor *nbr;
	struct in_addr ospf_mcast, *neighbors;
	int nbr_count=0, i=0;

	ospfheader = (struct ospfhdr *)packet;
	hello = (struct ospf_hello *)(packet+OSPFHDR_LEN);

	inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&ospf_mcast);
	if((to == ospf_mcast.s_addr) || (to == oiface->iface->ip.s_addr)) {
		add_neighbor(from,hello->network_mask.s_addr,ospfheader->src_router,oiface,hello->hello_interval,hello->options,hello->priority,hello->dead_interval);
		nbr_count = ((size-sizeof(struct ospfhdr)) - (sizeof(struct ospf_hello) - sizeof(struct in_addr)))/(sizeof(struct in_addr));
		if(nbr_count>0) {
			neighbors = (struct in_addr *)(packet + (size - (sizeof(struct in_addr)*nbr_count)));
			for(i=0;i<nbr_count;i++) {
				if(neighbors[i].s_addr == ospf0->router_id.s_addr) {
					nbr = find_neighbor_by_ip(to);
					if(nbr) send_dbdesc(nbr);
				}
			}
		}
	} else {
		// how did this packet get here
	}
}

void send_dbdesc(struct ospf_neighbor *nbr) {

}
