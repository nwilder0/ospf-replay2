/*
 * lsa.c
 *
 *  Created on: Jun 15, 2013
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
#include "lsa.h"

struct router_lsa* set_router_lsa() {
	struct router_lsa *this;
	struct ospf_lsa *lsa;
	int i;
	struct replay_list *tmp_item;
	struct ospf_neighbor *nbr;
	struct ospf_prefix *pfx;
	int links = ospf0->active_pfxcount + ospf0->nbrcount;
	u_int32_t ls_seqnum;

	if(ospf0->lsdb->this_rtr) {
		lsa = ospf0->lsdb->this_rtr;
		this = (struct router_lsa *)lsa->header;
		if(links != this->links) {
			ls_seqnum = htonl(ntohl(this->header.ls_seqnum) + 1);
			remove_lsa(lsa);
			this = (struct router_lsa *) malloc(sizeof(struct router_lsa) + sizeof(struct router_lsa_link)*(links-1));
			lsa = (struct ospf_lsa *) malloc(sizeof(struct ospf_lsa));
			lsa->header = (struct lsa_header *)this;
			this->header.ls_seqnum = ls_seqnum;
		}
	}
	else {
		this = (struct router_lsa *) malloc(sizeof(struct router_lsa) + sizeof(struct router_lsa_link)*(links-1));
		this->header.ls_seqnum = htonl(OSPF_INITIAL_SEQUENCE_NUMBER);
	}

	ospf0->lsdb->this_rtr = lsa;

	this->header.ls_age = htons(OSPF_LSA_INITIAL_AGE);
	this->header.adv_router.s_addr = ospf0->router_id.s_addr;
	this->header.id.s_addr = ospf0->router_id.s_addr;

	this->header.options = 0;
	this->header.type = OSPF_LSATYPE_ROUTER;

	this->links = ospf0->active_pfxcount + ospf0->nbrcount;
	this->flags = 0;
	i = 0;
	// add stub links
	tmp_item = ospf0->pflist;
	while(tmp_item) {
		if(tmp_item->object) {
			pfx = (struct ospf_prefix *) tmp_item->object;
			if(pfx->ospf_if) {
				this->link[i].link_id.s_addr = pfx->network.s_addr;
				this->link[i].link_data.s_addr = pfx->mask.s_addr;
				this->link[i].metric = get_if_metric(pfx->ospf_if);
				this->link[i].tos = 0;
				this->link[i].type = LSA_LINK_TYPE_STUB;
				i++;
			}
		}
		tmp_item = tmp_item->next;
	}

	tmp_item = ospf0->nbrlist;
	while(tmp_item) {
		if(tmp_item->object) {
			nbr = (struct ospf_neighbor *) tmp_item->object;
			if(nbr->state >= OSPF_NBRSTATE_2WAY) {
				this->link[i].link_id.s_addr = nbr->router_id.s_addr;
				this->link[i].link_data.s_addr = nbr->ip.s_addr;
				this->link[i].metric = get_if_metric(nbr->ospf_if);
				this->link[i].tos = 0;
				this->link[i].type = LSA_LINK_TYPE_POINTOPOINT;
				i++;
			}
		}
		tmp_item = tmp_item->next;
	}

	this->header.length = 20 + 4 + this->links * 16;
	this->header.checksum = ospf_lsa_checksum(&this->header);

	gettimeofday(&lsa->tv_recv,NULL);
	add_event((struct replay_object *)lsa,OSPF_EVENT_LSA_AGING);

	return this;
}

u_int16_t
ospf_lsa_checksum (struct lsa_header *lsa)
{
  u_char *buffer = (u_char *) &lsa->options;
  int options_offset = buffer - (u_char *) &lsa->ls_age; /* should be 2 */

  /* Skip the AGE field */
  u_int16_t len = ntohs(lsa->length) - options_offset;

  /* Checksum offset starts from "options" field, not the beginning of the
     lsa_header struct. The offset is 14, rather than 16. */
  int checksum_offset = (u_char *) &lsa->checksum - buffer;

  return fletcher_checksum(buffer, len, checksum_offset);
}

int
ospf_lsa_checksum_valid (struct lsa_header *lsa)
{
  u_char *buffer = (u_char *) &lsa->options;
  int options_offset = buffer - (u_char *) &lsa->ls_age; /* should be 2 */

  /* Skip the AGE field */
  u_int16_t len = ntohs(lsa->length) - options_offset;

  return(fletcher_checksum(buffer, len, FLETCHER_CHECKSUM_VALIDATE) == 0);
}

int add_lsa(struct lsa_header *header) {
	// see if already exists
	struct replay_list *tmp_item;
	struct ospf_lsa *tmp_lsa;
	struct lsa_header *tmp_header;
	int duplicate = FALSE;

	tmp_item = ospf0->lsdb->lsa_list[header->type];

	if(tmp_item) {
		tmp_lsa = (struct ospf_lsa*)tmp_item->object;
	}
	while(tmp_item && tmp_lsa && !(duplicate)) {
		tmp_header = (struct lsa_header *)tmp_lsa->header;
		tmp_item = tmp_item->next;
		if(tmp_header) {
			if((header->id.s_addr == tmp_header->id.s_addr) && (header->adv_router.s_addr == tmp_header->adv_router.s_addr)) {

				//check if only age has changed
				if(header->checksum == tmp_header->checksum) {
					duplicate = TRUE;
					gettimeofday(&tmp_lsa->tv_recv,NULL);
					tmp_header->ls_age = header->ls_age;
				}
				else {
					remove_lsa(tmp_lsa);
					tmp_item = NULL;
				}
			}
		}
		if(tmp_item) {
			tmp_lsa = (struct ospf_lsa*)tmp_item->object;
		}
	}

	if(!duplicate) {
		// add to appropriate array in lsdb
		struct replay_list *new_item = (struct replay_list *) malloc(sizeof(struct replay_list));
		struct ospf_lsa *new_lsa = (struct ospf_lsa *) malloc(sizeof(struct ospf_lsa));
		new_item->next = NULL;
		new_item->object = (struct replay_object *) new_lsa;
		new_lsa->header = header;
		gettimeofday(&new_lsa->tv_recv,NULL);
		ospf0->lsdb->lsa_list[header->type] = add_to_list(ospf0->lsdb->lsa_list[header->type],new_item);
		add_event((struct replay_object *)new_lsa,OSPF_EVENT_LSA_AGING);
	} else {
		add_event((struct replay_object *)tmp_lsa,OSPF_EVENT_LSA_AGING);
	}
	return duplicate;
}

void remove_lsa(struct ospf_lsa *lsa) {
	struct replay_list *item;
	struct ospf_event *event;
	item = find_in_list(ospf0->lsdb->lsa_list[lsa->header->type],(struct replay_object *)lsa);
	remove_from_list(ospf0->lsdb->lsa_list[lsa->header->type],item);
	event = find_event((struct replay_object *)lsa,OSPF_EVENT_LSA_AGING);
	remove_event(event);
	free(lsa->header);
	free(lsa);
}
