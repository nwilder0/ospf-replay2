/*
 * prefix.c
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


void add_prefix(char* full_string, u_int32_t area) {

	char net_string[16],mask_string[16];
	struct replay_interface *iface;
	struct in_addr net, mask;
	struct ospf_prefix *new_pfx, *tmp_pfx;
	struct replay_list *tmp_item;
	int duplicate = FALSE;

	// split up the network and mask
	strcpy(net_string,strtok(full_string,"/"));
	strcpy(mask_string,strtok(NULL,"/"));

	mask.s_addr = bits2mask(atoi(mask_string));

	inet_pton(AF_INET,net_string,&net);

	tmp_item = ospf0->pflist;
	if(tmp_item)
		tmp_pfx = (struct ospf_prefix *)tmp_item->object;
	while(tmp_pfx && tmp_item) {
		if((tmp_pfx->network.s_addr == net.s_addr) && (tmp_pfx->mask.s_addr == mask.s_addr)) {
			duplicate = TRUE;
			tmp_item = NULL;
		}
		else {
			tmp_item = tmp_item->next;
			if(!tmp_item)
				tmp_pfx = (struct ospf_prefix *)tmp_item->object;
		}
	}
	if(!duplicate) {
		iface = find_interface(net.s_addr,mask.s_addr);

		new_pfx = malloc(sizeof(*new_pfx));
		memset(new_pfx,0,sizeof(*new_pfx));

		new_pfx->mask.s_addr = mask.s_addr;
		new_pfx->network.s_addr = net.s_addr;
		new_pfx->iface = iface;
		new_pfx->area = area;

		ospf0->pflist = add_to_list(ospf0->pflist,(void *)new_pfx);
		ospf0->pfxcount++;

		if(iface && !ospf0->passif) {
			new_pfx->ospf_if = add_interface(iface, area);
			if(new_pfx->ospf_if) {

				ospf0->active_pfxcount++;
				if(new_pfx->ospf_if->nbrlist) {
					ospf0->transit_pfxcount++;
					new_pfx->type = OSPF_PFX_TYPE_TRANSIT;
				} else {
					ospf0->stub_pfxcount++;
					new_pfx->type = OSPF_PFX_TYPE_STUB;
				}
				duplicate = FALSE;
				tmp_item = new_pfx->ospf_if->pflist;
				if(tmp_item)
					tmp_pfx = (struct ospf_prefix *)tmp_item->object;
				while(tmp_pfx && tmp_item) {
					if((tmp_pfx->network.s_addr == new_pfx->network.s_addr) && (tmp_pfx->mask.s_addr == new_pfx->network.s_addr)) {
						duplicate = TRUE;
						tmp_item = NULL;
						tmp_pfx = NULL;
					}
					else {
						tmp_item = tmp_item->next;
						if(!tmp_item) {
							tmp_pfx = (struct ospf_prefix *)tmp_item->object;
						}
					}
				}
				if(!duplicate) {
					new_pfx->ospf_if->pflist = add_to_list(new_pfx->ospf_if->pflist,(void *)new_pfx);
				}
			}
		}
	}
	if(ospf0->started) {
		set_router_lsa();
	}
}


void remove_prefix(struct ospf_prefix *ospf_pfx) {

	struct replay_list *tmp_item;
	struct ospf_prefix *tmp_pfx;

	tmp_item = ospf0->pflist;
	while(tmp_item) {
		tmp_pfx = (struct ospf_prefix *)tmp_item->object;
		if(tmp_pfx==ospf_pfx) {
			ospf0->pflist = remove_from_list(ospf0->pflist,tmp_item);
			ospf0->pfxcount--;
			tmp_item = NULL;
		}
		else {
			tmp_item = tmp_item->next;
		}
	}
	if(ospf_pfx->ospf_if) {
		ospf0->active_pfxcount--;
		tmp_item = ospf_pfx->ospf_if->pflist;
		while(tmp_item) {
			tmp_pfx = (struct ospf_prefix *)tmp_item->object;
			if(tmp_pfx==ospf_pfx) {
				ospf_pfx->ospf_if->pflist = remove_from_list(ospf_pfx->ospf_if->pflist,tmp_item);
				tmp_item = NULL;
			}
			else {
				tmp_item = tmp_item->next;
			}
		}
		if(!ospf_pfx->ospf_if->pflist) {
			remove_interface(ospf_pfx->ospf_if);
		}
	}

	free(ospf_pfx);
	if(ospf0->started) {
		set_router_lsa();
	}

}

struct ospf_prefix* find_prefix(u_int32_t ip) {

	struct ospf_prefix *search_pfx, *found;
	struct replay_list *search_item;

	search_item = ospf0->pflist;
	found = NULL;

	while(search_item) {
		if(search_item->object) {
			search_pfx = (struct ospf_prefix *)search_item->object;
			if(ip_in_net(ip,search_pfx->network.s_addr,search_pfx->mask.s_addr)) {
				found = search_pfx;
			}
		}
		search_item = search_item->next;
	}
	return found;

}

