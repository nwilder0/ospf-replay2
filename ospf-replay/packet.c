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
		case OSPF_MESG_DBDESC:
			process_dbdesc(buffer+iphdr_len,ipheader->saddr,ipheader->daddr,((unsigned int)ntohs(ospfheader->packet_length)),oiface);
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
	size = sizeof(struct ospfhdr)+sizeof(struct ospf_hello)+((nbrcount-1)*sizeof(u_int32_t));
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
	free(packet);
}

void process_hello(void *packet, u_int32_t from, u_int32_t to, unsigned int size, struct ospf_interface *oiface) {
	struct ospfhdr *ospfheader;
	struct ospf_hello *hello;
	struct ospf_neighbor *nbr;
	struct in_addr ospf_mcast, *neighbors;
	int nbr_count=0, i=0;
	nbr = NULL;
	ospfheader = (struct ospfhdr *)packet;
	hello = (struct ospf_hello *)(packet+OSPFHDR_LEN);

	inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&ospf_mcast);
	if((to == ospf_mcast.s_addr) || (to == oiface->iface->ip.s_addr)) {
		add_neighbor(from,ospfheader->src_router,oiface,hello);
		nbr_count = ((size-sizeof(struct ospfhdr)) - (sizeof(struct ospf_hello) - sizeof(struct in_addr)))/(sizeof(struct in_addr));
		if(nbr_count>0) {
			neighbors = (struct in_addr *)(packet + (size - (sizeof(struct in_addr)*nbr_count)));
			for(i=0;i<nbr_count;i++) {
				if(neighbors[i].s_addr == ospf0->router_id.s_addr) {
					nbr = find_neighbor_by_ip(to);

					//if(nbr) send_dbdesc(nbr);
				}
			}
		}
		send_hello(oiface,NULL);
		if(nbr) {
			//check if adjacency should form
			/* - p2p
			* - this is DR
			* - this is BDR
			* - other is DR
			* - other is BDR
			*
			*/
			nbr->state = OSPF_NBRSTATE_2WAY;
			//dr-bdr negotiation: handles most scenarios but not RFC compliant
			if(oiface->dr.s_addr == 0) {
				if(nbr->dr.s_addr==0) {
					if(oiface->priority>nbr->priority) {
						oiface->dr.s_addr = ospf0->router_id.s_addr;
					} else if (nbr->priority>oiface->priority) {
						oiface->dr.s_addr = nbr->router_id.s_addr;
					} else {
						if(ntohl(ospf0->router_id.s_addr)<ntohl(nbr->router_id.s_addr)) {
							oiface->dr.s_addr = nbr->router_id.s_addr;
						} else {
							oiface->dr.s_addr = ospf0->router_id.s_addr;
						}
					}
				} else if (nbr->dr.s_addr == nbr->router_id.s_addr){
					oiface->dr.s_addr = nbr->router_id.s_addr;
				} else if (nbr->dr.s_addr == ospf0->router_id.s_addr){
					oiface->dr.s_addr = ospf0->router_id.s_addr;
				} else {
					//wait for dr to talk
				}
				send_hello(oiface,NULL);
			} else if (oiface->bdr.s_addr == 0){
				//figure out who is bdr
				if (nbr->bdr.s_addr == nbr->router_id.s_addr) {
					oiface->bdr.s_addr = nbr->bdr.s_addr;
				}
			}
			if((oiface->dr.s_addr == ospf0->router_id.s_addr)||(oiface->bdr.s_addr==ospf0->router_id.s_addr)||(nbr->dr.s_addr==nbr->router_id.s_addr)||(nbr->bdr.s_addr==nbr->router_id.s_addr)) {
				//if so, go to exstart
				nbr->state = OSPF_NBRSTATE_EXSTART;
				nbr->lsa_send_list = copy_lsalist();
				nbr->lsa_send_count = ospf0->lsdb->count;
				send_dbdesc(nbr,0);
			}



		}

	} else {
		// how did this packet get here
	}
}

void send_dbdesc(struct ospf_neighbor *nbr,u_int32_t db_seq_num) {
//if exstart send empty dbdesc w/ I,M,MS bits set
// if exchange send full db
	struct in_addr src_addr, remote_addr;
	void *packet=NULL;
	struct ospf_dbdesc *dbdesc;
	struct replay_list *tmp_item;
	int size, send_count;
	struct timeval tv;
	struct lsa_header *lsahdr;

	src_addr.s_addr = nbr->ospf_if->iface->ip.s_addr;
	remote_addr.s_addr = nbr->ip.s_addr;

	if(nbr->state==OSPF_NBRSTATE_EXSTART) {
		size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc);
		packet = malloc(size);
		dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
		dbdesc->flags = OSPF_DBDESC_FLAG_INIT + OSPF_DBDESC_FLAG_MORE + OSPF_DBDESC_FLAG_MASTER;
		gettimeofday(&tv,NULL);
		dbdesc->dd_seqnum = (u_int32_t)(tv.tv_sec);

	} else if (nbr->state==OSPF_NBRSTATE_EXCHANGE) {
		if(nbr->master) {
			//if received ack, send next dbdesc
			if(db_seq_num==nbr->last_sent_seq) {
				if(nbr->lsa_send_count) {

					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
					packet = malloc(size);
					bzero((char *) &packet, size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = OSPF_DBDESC_FLAG_MORE + nbr->master;
					dbdesc->dd_seqnum = ++(nbr->last_sent_seq);
					tmp_item = nbr->lsa_send_list;

					for(send_count=nbr->lsa_send_count;send_count;send_count--) {
						if(tmp_item) {
							lsahdr = (struct lsa_header *)tmp_item->object;
							memcpy(packet+(size-(sizeof(struct lsa_header))*send_count),lsahdr,sizeof(struct lsa_header));
							tmp_item = tmp_item->next;
						}
					}
					nbr->lsa_send_count = send_count;

				} else {
					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc);
					packet = malloc(size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = nbr->master;
					dbdesc->dd_seqnum = ++(nbr->last_sent_seq);
				}
			// if no ack, resent last dbdesc
			} else {
				if(nbr->lsa_send_list) {
					nbr->lsa_send_count = ospf0->lsdb->count;
					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
					packet = malloc(size);
					bzero((char *) &packet, size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = OSPF_DBDESC_FLAG_MORE + nbr->master;
					dbdesc->dd_seqnum = nbr->last_sent_seq;
					tmp_item = nbr->lsa_send_list;

					for(send_count=nbr->lsa_send_count;send_count;send_count--) {
						if(tmp_item) {
							lsahdr = (struct lsa_header *)tmp_item->object;
							memcpy(packet+(size-(sizeof(struct lsa_header))*send_count),lsahdr,sizeof(struct lsa_header));
							tmp_item = tmp_item->next;
						}
					}
					nbr->lsa_send_count = 0;

				} else {
					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc);
					packet = malloc(size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = nbr->master;
					dbdesc->dd_seqnum = nbr->last_sent_seq;
				}
			}

		} else {
			if(db_seq_num) {
				//send ack
				size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc);
				packet = malloc(size);
				dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
				dbdesc->flags = nbr->master;
				dbdesc->dd_seqnum = db_seq_num;

			} else {
				//send initial full dbdesc
				size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
				packet = malloc(size);
				bzero((char *) &packet, size);
				dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
				dbdesc->flags = OSPF_DBDESC_FLAG_MORE + nbr->master;
				dbdesc->dd_seqnum = nbr->last_recv_seq;
				tmp_item = nbr->lsa_send_list;

				for(send_count=nbr->lsa_send_count;send_count;send_count--) {
					if(tmp_item) {
						lsahdr = (struct lsa_header *)tmp_item->object;
						memcpy(packet+(size-(sizeof(struct lsa_header))*send_count),lsahdr,sizeof(struct lsa_header));
						tmp_item = tmp_item->next;
					}
				}
				nbr->lsa_send_count = 0;

			}
		}
	}
	if(packet) {
		dbdesc->options = ospf0->options;
		dbdesc->mtu = nbr->ospf_if->iface->mtu;
		nbr->last_sent_seq = dbdesc->dd_seqnum;

		build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_DBDESC,packet,size,nbr->ospf_if);
		send_packet(nbr->ospf_if,packet,remote_addr.s_addr,size);
	}

}

void process_dbdesc(void *packet, u_int32_t from, u_int32_t to, unsigned int size, struct ospf_interface *oiface) {
	struct ospf_neighbor *nbr=NULL;
	struct ospf_dbdesc *dbdesc;
	struct ospfhdr *ospf_header;
	int lsa_hdrs;
	struct lsa_header *lsahdr,*need_lsa;

	nbr=find_neighbor_by_ip(from);
	if(nbr) {
		if(nbr->ospf_if != oiface) {
			nbr = NULL;
		}
	}

	if(packet && nbr) {
		ospf_header = (struct ospfhdr *)(packet);
		if(ospf_header->mesg_type == OSPF_MESG_DBDESC) {
			dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
			if(dbdesc) {
				nbr->last_recv_seq = dbdesc->dd_seqnum;
				if(nbr->state == OSPF_NBRSTATE_EXSTART) {
					//calc who is master
					if(dbdesc->flags == OSPF_DBDESC_FLAG_MORE + OSPF_DBDESC_FLAG_INIT + OSPF_DBDESC_FLAG_MASTER) {
						//move to exchange
						//if slave send dbdesc
						nbr->state = OSPF_NBRSTATE_EXCHANGE;
						if(ntohl(nbr->router_id.s_addr) < ntohl(ospf0->router_id.s_addr)) {
							nbr->master = OSPF_DBDESC_FLAG_MASTER;
						} else {
							send_dbdesc(nbr,0);
						}
					}

				} else if (nbr->state==OSPF_NBRSTATE_EXCHANGE) {
					if(CHECK_BIT(dbdesc->flags,OSPF_DBDESC_FLAG_MORE)) {
						//get the lsa hdrs and check which are needed
						lsa_hdrs = size - sizeof(struct ospfhdr) - sizeof(struct ospf_dbdesc);
						while(lsa_hdrs) {
							lsahdr = (struct lsa_header *)(packet + (size-lsa_hdrs));
								if(!have_lsa(lsahdr)) {
									//add to needed lsa list
									need_lsa = (struct lsa_header *) malloc(sizeof(struct lsa_header));
									nbr->lsa_need_list = add_to_list(nbr->lsa_need_list,(struct replay_object *)need_lsa);
									nbr->lsa_need_count++;
								}
							lsa_hdrs = lsa_hdrs - sizeof(struct lsa_header);
						}
					} else {

						if(nbr->master) {
							if((nbr->last_sent_seq==nbr->last_recv_seq)&&nbr->lsa_send_count==0) {
								nbr->state = OSPF_NBRSTATE_LOADING;
							}

						} else {
							nbr->state = OSPF_NBRSTATE_LOADING;

						}
					}
					if(!(nbr->master&&nbr->state==OSPF_NBRSTATE_LOADING)) {
						send_dbdesc(nbr,nbr->last_recv_seq);
					}

					//if not MORE and slave then send ack, move to loading

					//if master and first slave packet, checked needed lsas, and send dbdesc

					//if master and ack, send next

					//if master and received ack to last then move to loading
				}
				if(nbr->state == OSPF_NBRSTATE_LOADING && nbr->lsa_need_list) {
					send_lsr(nbr);
				}
			}
		}
	}
}

void send_lsr(struct ospf_neighbor *nbr) {

}
