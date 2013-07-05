/*
 * neighbor.c
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

void add_neighbor(u_int32_t ip,u_int32_t router_id,struct ospf_interface *ospf_if,struct ospf_hello *hello) {
	struct ospf_neighbor *nbr;

	nbr = find_neighbor_by_ip(ip);
	// make sure if matches?
	if(nbr) {
		//if neighbor exists, update dead time
		gettimeofday(&nbr->last_heard,NULL);
		add_event((struct replay_object*)nbr,OSPF_EVENT_NBR_DEAD);
	} else {
		//else create new neighbor
		nbr = (struct ospf_neighbor *)malloc(sizeof(struct ospf_neighbor));
		nbr->dead_interval = hello->dead_interval;
		nbr->hello_interval = hello->hello_interval;
		nbr->ip.s_addr = ip;
		nbr->mask.s_addr = hello->network_mask.s_addr;
		nbr->options = hello->options;
		nbr->ospf_if = ospf_if;
		nbr->priority = hello->priority;
		nbr->router_id.s_addr = router_id;
		nbr->bdr.s_addr = hello->bdr.s_addr;
		nbr->dr.s_addr = hello->dr.s_addr;
		nbr->state = OSPF_NBRSTATE_INIT;
		gettimeofday(&nbr->last_heard,NULL);
		add_event((struct replay_object*)nbr,OSPF_EVENT_NBR_DEAD);

		ospf0->nbrlist = add_to_list(ospf0->nbrlist,(struct replay_object *)nbr);
		ospf0->nbrcount++;

		ospf_if->nbrlist = add_to_list(ospf_if->nbrlist,(struct replay_object *)nbr);

	}
}

void remove_neighbor(struct ospf_neighbor *nbr) {
	struct replay_list *item;

	remove_object_events((struct replay_object *)nbr);

	item = find_in_list(ospf0->nbrlist,(struct replay_object *)nbr);
	if(item) {
		ospf0->nbrlist = remove_from_list(ospf0->nbrlist,item);
		ospf0->nbrcount--;
	}

	item = find_in_list(nbr->ospf_if->nbrlist,(struct replay_object *)nbr);
	if(item) nbr->ospf_if->nbrlist = remove_from_list(ospf0->nbrlist,item);

	// what about LSAs from this neighbor?

	free(nbr);
}

struct ospf_neighbor* find_neighbor_by_ip(u_int32_t ip) {
	struct replay_list *curr;
	struct ospf_neighbor *nbr,*found;

	found = NULL;
	curr = ospf0->nbrlist;

	while(curr && !found) {
		nbr = (struct ospf_neighbor *)curr->object;
		if(nbr) {
			if(nbr->ip.s_addr==ip) {
				found = nbr;
			}
		}
		curr = curr->next;
	}
	return found;
}
