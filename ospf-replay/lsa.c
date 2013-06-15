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
	int i;
	struct replay_list *tmp_item;
	struct ospf_neighbor *nbr;
	struct ospf_prefix *pfx;

	if(ospf0->lsdb->this_rtr) {
		this = ospf0->lsdb->this_rtr;
		this->header.ls_seqnum = htonl(ntohl(this->header.ls_seqnum) + 1);
	}
	else {
		this = (struct router_lsa *) malloc(sizeof(struct router_lsa));
		this->header.ls_seqnum = htonl(OSPF_INITIAL_SEQUENCE_NUMBER);
	}

	this->header.ls_age = htons(OSPF_LSA_INITIAL_AGE);
	this->header.adv_router.s_addr = ospf0->router_id.s_addr;
	this->header.id.s_addr = ospf0->router_id.s_addr;

	this->header.options = 0;
	this->header.type = OSPF_LSATYPE_ROUTER;

	this->links = ospf0->active_pfxcount + ospf0->nbrcount;
	this->flags = 0;
	this->link = (struct router_lsa_link *) malloc(this->links * sizeof(struct router_lsa_link));
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

