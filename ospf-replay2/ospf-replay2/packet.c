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

	buffer = malloc(sizeof(*buffer)*REPLAY_PACKET_BUFFER);
	memset(&recv_addr,0,sizeof(recv_addr));
	recv_addr_len = sizeof(recv_addr);

	size = recvfrom(socket, buffer, REPLAY_PACKET_BUFFER, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
	if(size>0) {
		oiface = find_oiface_by_socket(socket);
		if(recv_addr.sin_addr.s_addr != oiface->iface->ip.s_addr) {
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
			case OSPF_MESG_LSR:
				process_lsr(buffer+iphdr_len,ipheader->saddr,ipheader->daddr,((unsigned int)ntohs(ospfheader->packet_length)),oiface);
				break;
			case OSPF_MESG_LSU:
				process_lsu(buffer+iphdr_len,ipheader->saddr,ipheader->daddr,((unsigned int)ntohs(ospfheader->packet_length)),oiface);
				break;
			case OSPF_MESG_LSACK:
				process_lsack(buffer+iphdr_len,ipheader->saddr,ipheader->daddr,((unsigned int)ntohs(ospfheader->packet_length)),oiface);
				break;
			}
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
	size = sizeof(struct ospfhdr)+sizeof(struct ospf_hello)+((nbrcount-1)*sizeof(struct in_addr));
	packet = malloc(size);
	memset(packet,0,size);

	hello = (struct ospf_hello *)(packet + sizeof(struct ospfhdr));

	hello->bdr.s_addr = ospf_if->bdr.s_addr;
	hello->dead_interval = htonl(ospf0->dead_interval);
	hello->dr.s_addr = ospf_if->dr.s_addr;
	hello->hello_interval = htons(ospf0->hello_interval);
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
	header->packet_length = htons(size);
	header->src_router = ospf0->router_id.s_addr;
	header->version = OSPF_VERSION;
	header->auth_type = ospf_if->auth_type;
	memcpy(header->auth_data,ospf_if->auth_data,sizeof(header->auth_data));
	header->checksum = in_cksum((void *)header,ntohs(header->packet_length));

}


void send_packet(struct ospf_interface *ospf_if,void *packet,u_int32_t remote_addr,int size) {
	int error;
	struct sockaddr_in dest_addr;

	memset(&dest_addr,0,sizeof(dest_addr));

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
					nbr = find_neighbor_by_ip(from);

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
			// set the nbr state to 2way since hello has been seen
			if(nbr->state<=OSPF_NBRSTATE_2WAY) {
				nbr->state = OSPF_NBRSTATE_2WAY;

				//dr-bdr negotiation: handles most scenarios but not RFC compliant
				//if the dr is empty
				if(oiface->dr.s_addr == 0) {
					// figure out who is dr
					// if the nbr doesn't know...
					if(nbr->dr.s_addr==0) {
						//set it to whoever has larger priority or if equal then to greater router id
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
					//if nbr has a dr of itself then use it
					} else if (nbr->dr.s_addr == nbr->router_id.s_addr){
						oiface->dr.s_addr = nbr->router_id.s_addr;
					//if nbr has a dr of this rtr then go ahead and use ourself as dr
					} else if (nbr->dr.s_addr == ospf0->router_id.s_addr){
						oiface->dr.s_addr = ospf0->router_id.s_addr;
					} else {
						//wait for dr to talk
					}
					send_hello(oiface,NULL);
				// if the bdr is empty
				} else if (oiface->bdr.s_addr == 0){
					//figure out who is bdr
					//if the nbr thinks it is the bdr believe it
					if (nbr->bdr.s_addr == nbr->router_id.s_addr) {
						oiface->bdr.s_addr = nbr->bdr.s_addr;
					}
				}
				if(nbr->dr.s_addr&&(nbr->dr.s_addr!=oiface->dr.s_addr)) {
					oiface->dr.s_addr = nbr->dr.s_addr;
				}

				//if self or nbr is dr or bdr then go to exstart and send dbdesc

				if((oiface->dr.s_addr == oiface->iface->ip.s_addr)||(oiface->bdr.s_addr==oiface->iface->ip.s_addr)||(nbr->dr.s_addr==nbr->ip.s_addr)||(nbr->bdr.s_addr==nbr->ip.s_addr)) {
					//if so, go to exstart
					nbr->state = OSPF_NBRSTATE_EXSTART;
					nbr->lsa_send_list = copy_lsalist();
					nbr->lsa_send_count = ospf0->lsdb->count;
					send_dbdesc(nbr,0);
				}

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
		memset(packet,0,size);
		dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
		dbdesc->flags = OSPF_DBDESC_FLAG_INIT + OSPF_DBDESC_FLAG_MORE + OSPF_DBDESC_FLAG_MASTER;
		gettimeofday(&tv,NULL);
		dbdesc->dd_seqnum = (u_int32_t)(tv.tv_sec);

	} else if (nbr->state>=OSPF_NBRSTATE_EXCHANGE) {
		if(nbr->this_rtr_master) {
			//if received ack, send next dbdesc
			if(db_seq_num==nbr->last_sent_seq) {
				if(nbr->lsa_send_count) {

					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
					packet = malloc(size);
					memset(packet,0,size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = OSPF_DBDESC_FLAG_MORE + nbr->this_rtr_master;
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
					memset(packet,0,size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = nbr->this_rtr_master;
					dbdesc->dd_seqnum = ++(nbr->last_sent_seq);
				}
			// if no ack, resend last dbdesc
			} else {
				if(nbr->lsa_send_list) {
					nbr->lsa_send_count = ospf0->lsdb->count;
					size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
					packet = malloc(size);
					memset(packet,0,size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = OSPF_DBDESC_FLAG_MORE + nbr->this_rtr_master;
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
					memset(packet,0,size);
					dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
					dbdesc->flags = nbr->this_rtr_master;
					dbdesc->dd_seqnum = nbr->last_sent_seq;
				}
			}
		//if this rtr is slave
		} else {
			if(db_seq_num) {
				//send ack
				size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc);
				packet = malloc(size);
				memset(packet,0,size);
				dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
				dbdesc->flags = nbr->this_rtr_master;
				dbdesc->dd_seqnum = db_seq_num;
				nbr->last_sent_seq = dbdesc->dd_seqnum;

			} else {
				//send initial full dbdesc
				size = sizeof(struct ospfhdr)+sizeof(struct ospf_dbdesc)+sizeof(struct lsa_header)*(nbr->lsa_send_count);
				packet = malloc(size);
				memset(packet,0,size);
				dbdesc = (struct ospf_dbdesc *)(packet + sizeof(struct ospfhdr));
				dbdesc->flags = nbr->this_rtr_master;
				dbdesc->dd_seqnum = nbr->last_recv_seq;
				nbr->last_sent_seq = dbdesc->dd_seqnum;
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
		dbdesc->mtu = htons(nbr->ospf_if->iface->mtu);
		nbr->last_sent_seq = dbdesc->dd_seqnum;

		build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_DBDESC,packet,size,nbr->ospf_if);
		send_packet(nbr->ospf_if,packet,remote_addr.s_addr,size);
		if(nbr->this_rtr_master) {
			add_event((void *)nbr,OSPF_EVENT_DBDESC_RETX);
		}
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
				if((nbr->last_recv_seq==nbr->last_sent_seq)&&(nbr->this_rtr_master)) {
					//find retx event and remove it
					find_and_remove_event((void *)nbr,OSPF_EVENT_DBDESC_RETX);
				}
				if(nbr->state == OSPF_NBRSTATE_EXSTART) {
					//calc who is master
					if(dbdesc->flags == OSPF_DBDESC_FLAG_MORE + OSPF_DBDESC_FLAG_INIT + OSPF_DBDESC_FLAG_MASTER) {
						//move to exchange
						//if slave send dbdesc
						nbr->state = OSPF_NBRSTATE_EXCHANGE;
						if(ntohl(nbr->router_id.s_addr) < ntohl(ospf0->router_id.s_addr)) {
							nbr->this_rtr_master = OSPF_DBDESC_FLAG_MASTER;
						} else {
							send_dbdesc(nbr,0);
						}
					}

				} else if (nbr->state==OSPF_NBRSTATE_EXCHANGE) {
					//get the lsa hdrs and check which are needed
					lsa_hdrs = size - sizeof(struct ospfhdr) - sizeof(struct ospf_dbdesc);
					while(lsa_hdrs) {
						lsahdr = (struct lsa_header *)(packet + (size-lsa_hdrs));
							if(!have_lsa(lsahdr)) {
								//add to needed lsa list
								need_lsa = malloc(sizeof(*need_lsa));
								memset(need_lsa,0,sizeof(*need_lsa));
								memcpy(need_lsa,lsahdr,sizeof(struct lsa_header));
								nbr->lsa_need_list = add_to_list(nbr->lsa_need_list,(void *)need_lsa);
								nbr->lsa_need_count++;
							}
						lsa_hdrs = lsa_hdrs - sizeof(struct lsa_header);
					}

					if(CHECK_BIT(dbdesc->flags,OSPF_DBDESC_FLAG_MORE)) {

					} else {

						if(nbr->this_rtr_master) {
							if((nbr->last_sent_seq==nbr->last_recv_seq)&&nbr->lsa_send_count==0) {
								nbr->state = OSPF_NBRSTATE_LOADING;
							}

						} else {
							nbr->state = OSPF_NBRSTATE_LOADING;

						}
					}

					if(!(nbr->this_rtr_master)) {
						if((dbdesc->flags!=OSPF_DBDESC_FLAG_MORE + OSPF_DBDESC_FLAG_INIT + OSPF_DBDESC_FLAG_MASTER)&&(nbr->last_sent_seq!=dbdesc->dd_seqnum))
							send_dbdesc(nbr,nbr->last_recv_seq);
					} else if(!(nbr->state==OSPF_NBRSTATE_LOADING)){
						send_dbdesc(nbr,nbr->last_recv_seq);
					}

					//if not MORE and slave then send ack, move to loading

					//if master and first slave packet, checked needed lsas, and send dbdesc

					//if master and ack, send next

					//if master and received ack to last then move to loading
				}
				if(nbr->state == OSPF_NBRSTATE_LOADING) {
					if(nbr->lsa_send_list) {
						nbr->lsa_send_list = remove_all_from_list(nbr->lsa_send_list);
					}
					nbr->lsa_send_count = 0;
					nbr->last_recv_seq = 0;
					nbr->last_sent_seq = 0;
					nbr->this_rtr_master = 0;
					if(nbr->lsa_need_list) {
						send_lsr(nbr);
					} else {
						nbr->state = OSPF_NBRSTATE_FULL;
					}
				}
			}
		}
	}
}

void send_lsr(struct ospf_neighbor *nbr) {

	struct lsa_header *lsahdr;
	struct ospf_lsr *lsr;
	void *packet;
	int size;
	struct in_addr src_addr,remote_addr;

	if(nbr->lsa_need_list) {

		src_addr.s_addr = nbr->ospf_if->iface->ip.s_addr;
		remote_addr.s_addr = nbr->ip.s_addr;

		lsahdr = (struct lsa_header *)(nbr->lsa_need_list->object);

		size = sizeof(struct ospfhdr) + sizeof(struct ospf_lsr);
		packet = malloc(size);
		memset(packet,0,size);

		lsr = (struct ospf_lsr *)(packet+sizeof(struct ospfhdr));
		lsr->advert_rtr = lsahdr->adv_router.s_addr;
		lsr->lsa_id = lsahdr->id.s_addr;
		lsr->lsa_type = htonl((u_int32_t)(lsahdr->type));

		build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_LSR,packet,size,nbr->ospf_if);
		send_packet(nbr->ospf_if,packet,remote_addr.s_addr,size);
		//add event to wait for LSU or resend lsr
		add_event((void *)nbr,OSPF_EVENT_LSR_RETX);

	} else {
		nbr->state = OSPF_NBRSTATE_FULL;
	}
}

void process_lsr(void *packet,u_int32_t from,u_int32_t to,unsigned int size,struct ospf_interface *oiface) {
	struct ospf_lsr *lsr;
	struct ospf_lsa *lsa;
	struct ospf_neighbor *nbr;
	struct replay_nlist *tmp_item=NULL;

	//find the neighbor who sent the packet
	nbr=find_neighbor_by_ip(from);
	// if nbr found
	if(nbr) {
		// if the found nbr is not on the interface that we received the packet from then ignore this packet
		if(nbr->ospf_if != oiface) {
			nbr = NULL;
		}
	}
	// make sure packet exists, valid nbr found, and packet size is correct for LSR
	if(packet && (size>=(sizeof(struct ospfhdr)+sizeof(struct ospf_lsr))) && nbr) {
		// get the LSR portion of the packet
		lsr = (struct ospf_lsr *)(packet+sizeof(struct ospfhdr));
		// find the lsa the nbr is asking for
		lsa = find_lsa(lsr->advert_rtr,lsr->lsa_id,ntohl(lsr->lsa_type));
		// if lsa found and the nbr is supposed to be sending LSRs
		if(lsa&&(nbr->state>=OSPF_NBRSTATE_EXSTART)) {
			// create a one item nlist with lsa as the object and the lsa id as key
			tmp_item = add_to_nlist(tmp_item,(void *)lsa,(unsigned long long)(lsa->header->id.s_addr));
			// send the lsu
			send_lsu(nbr,tmp_item,OSPF_LSU_NOTRETX);
			// remove the nlist items (but not the lsa objects)
			remove_all_from_nlist(tmp_item);
		}
	}
}

void send_lsu(struct ospf_neighbor *nbr,struct replay_nlist *lsalist,u_int8_t retx) {
	struct replay_nlist *tmp_nitem;
	struct replay_list *tmp_item;
	struct ospf_lsa *tmp_lsa;
	struct lsa_header *lsahdr;
	struct ospf_lsu *lsu;
	void *packet;
	int size, lsa_count=0;
	long long lsa_ptr=0;
	struct in_addr src_addr,remote_addr;
	struct ospf_neighbor *tmp_nbr;

	if(!lsalist) {
		lsalist = nbr->lsu_lsa_list;
	}

	if(nbr&&lsalist) {
		src_addr.s_addr = nbr->ospf_if->iface->ip.s_addr;
		if(retx) {
			remote_addr.s_addr = nbr->ip.s_addr;
		} else {
			inet_pton(AF_INET,OSPF_MULTICAST_ALLROUTERS,&remote_addr);
		}

		size = sizeof(struct ospfhdr) + sizeof(struct ospf_lsu);

		tmp_nitem = lsalist;
		while(tmp_nitem) {

			tmp_lsa = (struct ospf_lsa *)(tmp_nitem->object);
			if(tmp_lsa) {
				lsahdr = tmp_lsa->header;
				if(lsahdr) {
					lsa_count++;
					size = size + ntohs(lsahdr->length);
				}
			}
			tmp_nitem = tmp_nitem->next;
		}

		packet = malloc(size);
		memset(packet,0,size);
		lsu = (struct ospf_lsu *)(packet+sizeof(struct ospfhdr));

		//build lsu packet
		lsu->lsa_num = htonl(lsa_count);
		lsa_ptr = sizeof(struct ospfhdr)+sizeof(struct ospf_lsu);
		tmp_nitem = lsalist;

		while(tmp_nitem) {

			tmp_lsa = (struct ospf_lsa *)(tmp_nitem->object);
			if(tmp_lsa) {
				lsahdr = tmp_lsa->header;
				if(lsahdr) {
					memcpy(packet+lsa_ptr,lsahdr,ntohs(lsahdr->length));
					lsa_ptr = lsa_ptr + ntohs(lsahdr->length);
				}
			}
			tmp_nitem = tmp_nitem->next;
		}

		build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_LSU,packet,size,nbr->ospf_if);
		send_packet(nbr->ospf_if,packet,remote_addr.s_addr,size);

		//add event to wait for LSACK or resend unicast LSU
		if(retx) {
			nbr->lsu_lsa_list = merge_nlist(nbr->lsu_lsa_list,lsalist);
			add_event((void *)nbr,OSPF_EVENT_LSU_ACK);
		} else {
			tmp_item = nbr->ospf_if->nbrlist;
			while(tmp_item) {
				tmp_nbr = (struct ospf_neighbor *)(tmp_item->object);
				if(tmp_nbr) {
					if(tmp_nbr->state>=OSPF_NBRSTATE_EXSTART) {
						tmp_nbr->lsu_lsa_list = merge_nlist(nbr->lsu_lsa_list,lsalist);
						add_event((void *)tmp_nbr,OSPF_EVENT_LSU_ACK);
					}
				}
				tmp_item = tmp_item->next;
			}

		}
	}

}

void process_lsu(void *packet,u_int32_t from,u_int32_t to,unsigned int size,struct ospf_interface *oiface) {
	struct ospf_lsa *lsa;
	struct lsa_header *lsahdr,*new_hdr,*need_hdr;
	struct ospf_neighbor *nbr,*tmp_nbr;
	struct replay_nlist *updated=NULL;
	struct replay_list *tmp_item,*tmp_item2;
	long long lsa_ptr=0;
	int ignore=0,send=0,changed_need_list=0;
	struct timeval now,orig;
	struct ospf_interface *tmp_if;

	// find the nbr who sent the packet and make sure their iface matches the packet iface
	nbr=find_neighbor_by_ip(from);
	if(nbr) {
		if(nbr->ospf_if != oiface) {
			nbr = NULL;
		}
	}
	gettimeofday(&now,NULL);
	// if nbr, interface and packet are valid
	if(nbr&&oiface&&packet) {
		//ignore packet if from nbr who's < exstart
		if(nbr->state>=OSPF_NBRSTATE_EXSTART) {
			//start with the first lsa hdr in the LSU
			lsa_ptr = (long long)packet + sizeof(struct ospfhdr) + sizeof(struct ospf_lsu);
			// while we haven't reached the end of the packet
			while(lsa_ptr<((long long)packet+size)) {
				// get the current lsa hdr
				lsahdr = (struct lsa_header *)lsa_ptr;
				// increment the lsahdr's age by the transmit delay
				lsahdr->ls_age = htons(ntohs(lsahdr->ls_age) + (u_int16_t)(ospf0->transmit_delay));
				// find an existing lsa for this hdr if one exists
				lsa = find_lsa(lsahdr->adv_router.s_addr,lsahdr->id.s_addr,lsahdr->type);
				// get the time the lsa was originated
				orig.tv_sec = now.tv_sec - lsahdr->ls_age;
				// if lsa was found and lsa in lsdb is within margin of lsa recieved in lsu then ignore it
				if(lsa) {
					if((lsa->tv_orig.tv_sec+REPLAY_LSA_AGE_MARGIN)>orig.tv_sec) {
						ignore = 1;
					// else remove the old lsa
					} else {
						//ensure proper removal out of any lists?
						remove_lsa(lsa);
					}
				}
				// if we're not ignoring the current lsu packet's lsa hdr then add it to the lsdb
				if(!ignore) {
					// malloc the new lsa
					new_hdr = malloc(ntohs(lsahdr->length));
					memset(new_hdr,0,ntohs(lsahdr->length));
					// copy the packet's current lsa into the new system lsa
					memcpy(new_hdr,lsahdr,ntohs(lsahdr->length));
					// add this lsa to the lsdb
					lsa = add_lsa(new_hdr);
					// if LSA recording is on for this interface, then write the lsa to disk
					if(oiface->iface->record) {
						fwrite(new_hdr,ntohs(new_hdr->length),1,oiface->iface->record);
					}
					// if successfully added then add the new lsa to the updated list
					if(lsa) {
						updated = add_to_nlist(updated,(void *)lsa,new_hdr->id.s_addr);
					}
					// find the new hdr in the need list and remove it if found
					tmp_item = nbr->lsa_need_list;
					while(tmp_item) {
						need_hdr = (struct lsa_header *)(tmp_item->object);
						if(need_hdr) {
							if((need_hdr->adv_router.s_addr==new_hdr->adv_router.s_addr)&&(need_hdr->id.s_addr==new_hdr->id.s_addr)&&(need_hdr->type==new_hdr->type)) {
								changed_need_list = 1;
								nbr->lsa_need_list = remove_from_list(nbr->lsa_need_list,tmp_item);
								tmp_item = NULL;
							}
						}
						if(tmp_item) {
							tmp_item = tmp_item->next;
						}
					}
				}
				// move to next lsa
				lsa_ptr = lsa_ptr + ntohs(lsahdr->length);
			}
			// ack the recieved lsu
			send_lsack(nbr,updated);

			if(changed_need_list) {

				// remove the LSR retransmit
				find_and_remove_event((void *)nbr,OSPF_EVENT_LSR_RETX);

				// if anything is still needed then send a new, updated lsr
				// otherwise move the nbr to FULL
				if(nbr->lsa_need_list) {
					send_lsr(nbr);
				} else {
					nbr->state = OSPF_NBRSTATE_FULL;
				}
			}

			// for each active ospf interface
			tmp_item = ospf0->iflist;
			while(tmp_item) {
				tmp_if = (struct ospf_interface *)(tmp_item->object);
				if(tmp_if) {
					// for each nbr in this iface
					tmp_item2 = tmp_if->nbrlist;
					send = 0;
					while(tmp_item2) {
						// get the current nbr from the list item
						tmp_nbr = (struct ospf_neighbor *)(tmp_item2->object);
						// if the nbr exists and is not the nbr who sent the lsu
						if(tmp_nbr && (tmp_nbr != nbr)) {
							// if the nbr is at a state to recieve LSAs
							if(tmp_nbr->state>=OSPF_NBRSTATE_EXSTART) {
								// then set the send flag
								send = 1;
								tmp_item2=NULL;
							}
						}
						// move to the next nbr
						if(tmp_item2) {
							tmp_item2 = tmp_item2->next;
						}
					}
				}
				// if a valid target for the lsu was found on this iface then send it
				if(send) {
					send_lsu(tmp_nbr,updated,OSPF_LSU_NOTRETX);
				}
				tmp_item = tmp_item->next;
			}

			remove_all_from_nlist(updated);
		}
	}
}

void send_lsack(struct ospf_neighbor *nbr, struct replay_nlist *lsalist) {
	struct replay_nlist *tmp_item;
	struct ospf_lsa *tmp_lsa;
	struct lsa_header *lsahdr;
	void *packet;
	int size, lsa_count=0;
	long long lsa_ptr=0;
	struct in_addr src_addr,remote_addr;

	if(nbr&&lsalist) {

		src_addr.s_addr = nbr->ospf_if->iface->ip.s_addr;
		remote_addr.s_addr = nbr->ip.s_addr;

		tmp_item = lsalist;
		while(tmp_item) {

			tmp_lsa = (struct ospf_lsa *)(tmp_item->object);
			if(tmp_lsa) {
				lsahdr = tmp_lsa->header;
				if(lsahdr) {
					lsa_count++;
				}
			}
			tmp_item = tmp_item->next;
		}
		size = sizeof(struct ospfhdr) + lsa_count*sizeof(struct lsa_header);

		packet = malloc(size);
		memset(packet,0,size);
		lsa_ptr = sizeof(struct ospfhdr);

		tmp_item = lsalist;
		while(tmp_item) {
			tmp_lsa = (struct ospf_lsa *)(tmp_item->object);
			if(tmp_lsa) {
				lsahdr = tmp_lsa->header;
				if(lsahdr) {
					memcpy(packet+lsa_ptr,lsahdr,sizeof(struct lsa_header));
					lsa_ptr = lsa_ptr + sizeof(struct lsa_header);
				}
			}
			tmp_item = tmp_item->next;
		}

		build_ospf_packet(src_addr.s_addr,remote_addr.s_addr,OSPF_MESG_LSACK,packet,size,nbr->ospf_if);
		send_packet(nbr->ospf_if,packet,remote_addr.s_addr,size);

	}

}

void process_lsack(void *packet,u_int32_t from,u_int32_t to,unsigned int size,struct ospf_interface *oiface) {
	long long lsa_ptr=0;
	struct lsa_header *hdr,*tmp_hdr;
	struct replay_nlist *tmp_item,*next;
	struct ospf_neighbor *nbr;
	struct ospf_lsa *tmp_lsa;

	nbr=find_neighbor_by_ip(from);
	if(nbr) {
		if(nbr->ospf_if != oiface) {
			nbr = NULL;
		}
	}

	if(packet&&size&&oiface&&nbr) {
		//get ptr to first lsa hdr
		lsa_ptr = (long long)packet + sizeof(struct ospfhdr);
		// while there is room for at least one more lsa hdr in the packet size...
		while((lsa_ptr+sizeof(struct lsa_header))<=((long long)packet+size)) {
			// get the current lsa hdr
			hdr = (struct lsa_header *)(lsa_ptr);
			// iterate thru the nbr's lsu lsa list to find and remove this hdr (since its been ack'd)
			tmp_item = nbr->lsu_lsa_list;
			while(tmp_item) {
				next = tmp_item->next;
				//get the current lsa in the lsu lsa list
				tmp_lsa = (struct ospf_lsa *)(tmp_item->object);
				if(tmp_lsa) {
					// get the current lsa hdr for this lsa in the lsu list
					tmp_hdr = tmp_lsa->header;
					if(tmp_hdr) {
						// if ack'd hdr checksum and current lsu list hdr checksum match...
						if(hdr->checksum==tmp_hdr->checksum) {
							// remove this current lsa from the lsu list, its been ack'd
							nbr->lsu_lsa_list = remove_from_nlist(nbr->lsu_lsa_list,tmp_item);
							tmp_item = NULL;
						} else {
							// else if we've gone past the ack'd hdr's potential position in the list go ahead and stop looking
							if(tmp_item->key>(unsigned long long)(hdr->id.s_addr)) {
								tmp_item = NULL;
							}
						}
					}
				}
				if(tmp_item) {
					tmp_item = next;
				}
			}
			// move to the next header in the LSACK packet (if any)
			lsa_ptr = lsa_ptr + sizeof(struct lsa_header);
		}

		// if lsu list is empty remove event
		if(nbr->lsu_lsa_list) {
			find_and_remove_event((void *)nbr,OSPF_EVENT_LSU_ACK);
		}

	}
}

