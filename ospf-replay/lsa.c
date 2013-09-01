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
	int i,size;
	struct replay_list *tmp_item;
	struct ospf_neighbor *nbr;
	struct ospf_prefix *pfx;
	int links = ospf0->stub_pfxcount + ospf0->nbrcount;
	u_int32_t ls_seqnum;

	if(ospf0->lsdb->this_rtr) {
		lsa = ospf0->lsdb->this_rtr;
		this = (struct router_lsa *)lsa->header;
		if(links != this->links) {
			ls_seqnum = htonl(ntohl(this->header.ls_seqnum) + 1);
			remove_lsa(lsa);

			//calculate the custom size for this since it may not match the type size due to varying number of links
			size = sizeof(struct router_lsa) + sizeof(struct router_lsa_link)*(links-1);
			this = malloc(size);
			memset(this,0,size);
			//store the custom size in LSA header length field as required by OSPF header structure
			// and for future reference by code if needed
			this->header.length = htons(size);
			this->header.ls_seqnum = ls_seqnum;

		}
	}
	else {
		//calculate the custom size for this since it may not match the type size due to varying number of links
		size = sizeof(struct router_lsa) + sizeof(struct router_lsa_link)*(links-1);
		this = malloc(size);
		memset(this,0,size);
		//store the custom size in LSA header length field as required by OSPF header structure
		// and for future reference by code if needed
		this->header.length = htons(size);
		this->header.ls_seqnum = htonl(OSPF_INITIAL_SEQUENCE_NUMBER);
	}

	lsa = malloc(sizeof(*lsa));
	memset(lsa,0,sizeof(*lsa));
	lsa->header = (struct lsa_header *)this;

	ospf0->lsdb->this_rtr = lsa;

	this->header.ls_age = htons(OSPF_LSA_INITIAL_AGE);
	this->header.adv_router.s_addr = ospf0->router_id.s_addr;
	this->header.id.s_addr = ospf0->router_id.s_addr;

	this->header.options = ospf0->options;
	this->header.type = OSPF_LSATYPE_ROUTER;

	this->links = htons(ospf0->stub_pfxcount + ospf0->nbrcount);
	this->flags = 0;
	i = 0;
	// add stub/transit links
	tmp_item = ospf0->pflist;
	while(tmp_item) {
		if(tmp_item->object) {
			pfx = (struct ospf_prefix *) tmp_item->object;
			if(pfx->ospf_if) {
				if(pfx->type == OSPF_PFX_TYPE_STUB) {
					this->link[i].link_id.s_addr = pfx->network.s_addr;
					this->link[i].link_data.s_addr = pfx->mask.s_addr;
					this->link[i].type = LSA_LINK_TYPE_STUB;
					this->link[i].metric = get_if_metric(pfx->ospf_if);
					this->link[i].tos = 0;

				}
				i++;
			}
		}
		tmp_item = tmp_item->next;
	}
	tmp_item = ospf0->nbrlist;
	while(tmp_item) {
		if(tmp_item->object) {
			nbr = (struct ospf_neighbor *) tmp_item->object;
			//if(nbr->state >= OSPF_NBRSTATE_2WAY) {
				this->link[i].link_id.s_addr = nbr->router_id.s_addr;
				this->link[i].link_data.s_addr = nbr->ip.s_addr;
				this->link[i].metric = get_if_metric(nbr->ospf_if);
				this->link[i].tos = 0;
				this->link[i].type = LSA_LINK_TYPE_TRANSIT;
				i++;
			//}
		}
		tmp_item = tmp_item->next;
	}

	this->header.checksum = ospf_lsa_checksum(&this->header);

	gettimeofday(&lsa->tv_recv,NULL);
	gettimeofday(&lsa->tv_orig,NULL);
	add_event((void *)lsa,OSPF_EVENT_LSA_AGING);
	add_lsa(ospf0->lsdb->this_rtr->header);

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

struct ospf_lsa* add_lsa(struct lsa_header *header) {
	// see if already exists
	struct replay_nlist *tmp_item;
	struct ospf_lsa *tmp_lsa;
	struct lsa_header *tmp_header;
	int duplicate = FALSE;
	struct ospf_lsa *new_lsa=NULL;

	tmp_item = ospf0->lsdb->lsa_list[header->type];

	if(tmp_item) {
		tmp_lsa = (struct ospf_lsa *)tmp_item->object;
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
					tmp_lsa->tv_orig.tv_sec = tmp_lsa->tv_recv.tv_sec - ntohs(tmp_header->ls_age);
					new_lsa = tmp_lsa;
				}
				else {
					remove_lsa(tmp_lsa);
					tmp_item = NULL;
				}
			}
			if(tmp_item) {
				if(header->id.s_addr < tmp_item->key) {
					tmp_item = NULL;
				}
			}
		}
		if(tmp_item) {
			tmp_lsa = (struct ospf_lsa *)tmp_item->object;
		}
	}

	if(!duplicate) {
		// add to appropriate array in lsdb
		if(ospf0->lsdb->this_rtr) {
			if(ospf0->lsdb->this_rtr->header == header) {
				new_lsa = ospf0->lsdb->this_rtr;
			}
		}
		if(!new_lsa) {
			new_lsa = malloc(sizeof(*new_lsa));
			memset(new_lsa,0,sizeof(*new_lsa));
			new_lsa->header = header;
			gettimeofday(&new_lsa->tv_recv,NULL);
			new_lsa->tv_orig.tv_sec = new_lsa->tv_recv.tv_sec - ntohs(new_lsa->header->ls_age);
		}


		ospf0->lsdb->lsa_list[header->type] = add_to_nlist(ospf0->lsdb->lsa_list[header->type],(void *)new_lsa,(unsigned long long)new_lsa->header->id.s_addr);
		ospf0->lsdb->count++;
		add_event((void *)new_lsa,OSPF_EVENT_LSA_AGING);
	} else {
		add_event((void *)tmp_lsa,OSPF_EVENT_LSA_AGING);
	}
	new_lsa->ip.s_addr = header->id.s_addr;
	new_lsa->hdr_ptr = &header;
	return new_lsa;
}

void remove_lsa(struct ospf_lsa *lsa) {
	struct replay_nlist *item;
	struct ospf_event *event;
	item = find_in_nlist(ospf0->lsdb->lsa_list[lsa->header->type],(void *)lsa);
	ospf0->lsdb->lsa_list[lsa->header->type] = remove_from_nlist(ospf0->lsdb->lsa_list[lsa->header->type],item);
	ospf0->lsdb->count--;
	event = find_event((void *)lsa,OSPF_EVENT_LSA_AGING);
	remove_event(event);
	free(lsa->header);
	free(lsa);
}

struct replay_list* copy_lsalist() {
	struct replay_list *start;
	struct replay_nlist *tmp_item;
	struct ospf_lsa *lsa;
	int i;
	start=NULL;
	for(i=0;i<OSPF_LSA_TYPES;i++) {
		tmp_item = ospf0->lsdb->lsa_list[i];
		while(tmp_item) {
			lsa = (struct ospf_lsa *)tmp_item->object;
			if(lsa) {
				if(lsa->header) {
					start = add_to_list(start,lsa->header);
				}
			}
			tmp_item = tmp_item->next;
		}
	}
	return start;
}

int have_lsa(struct lsa_header *lsahdr) {
	int have = 0;
	struct replay_nlist *tmp_item;
	struct lsa_header *tmphdr=NULL;
	struct ospf_lsa *tmplsa;
	struct timeval now,orig;

	if(lsahdr) {
		tmp_item = ospf0->lsdb->lsa_list[lsahdr->type];
		while(tmp_item) {
			tmplsa = (struct ospf_lsa *)(tmp_item->object);
			if(tmplsa) {
				tmphdr = tmplsa->header;
				if(tmphdr) {
					if((lsahdr->id.s_addr == tmphdr->id.s_addr)&&(lsahdr->adv_router.s_addr == tmphdr->adv_router.s_addr)) {
						gettimeofday(&now,NULL);
						orig.tv_sec = now.tv_sec - ntohs(lsahdr->ls_age);
						if((tmplsa->tv_orig.tv_sec+REPLAY_LSA_AGE_MARGIN)>orig.tv_sec) {
							have = 1;
						}
					}
					if((tmp_item->key > (unsigned long long)(lsahdr->id.s_addr))||have) {
						tmp_item = NULL;
					}
				}
			}
			if(tmp_item) {
				tmp_item = tmp_item->next;
			}
			tmphdr = NULL;
		}
	}

	return have;
}

struct ospf_lsa* find_lsa(u_int32_t adv_router, u_int32_t id, u_char type) {
	struct replay_nlist *tmp_item;
	struct lsa_header *tmphdr=NULL;
	struct ospf_lsa *found=NULL,*tmplsa;

	tmp_item = ospf0->lsdb->lsa_list[type];
	while(tmp_item) {
		tmplsa = (struct ospf_lsa *)tmp_item->object;
		if(tmplsa) {
			tmphdr = tmplsa->header;
			if(tmphdr) {
				if((id == tmphdr->id.s_addr)&&(adv_router == tmphdr->adv_router.s_addr)) {
					found = tmplsa;
				}
				if((tmp_item->key > (unsigned long long)id)||found) {
					tmp_item = NULL;
				}
			}
		}
		if(tmp_item) {
			tmp_item = tmp_item->next;
		}
		tmphdr = NULL;
	}

	return found;

}
