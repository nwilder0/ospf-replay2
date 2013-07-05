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

void add_neighbor(u_int32_t ip,u_int32_t mask,u_int32_t router_id,struct ospf_interface *ospf_if,u_short hello_interval,u_char options,u_char priority,u_int32_t dead_interval) {
	struct ospf_neighbor *nbr;
	struct replay_list *new_item;

	nbr = find_neighbor_by_ip(ip);
	// make sure if matches?
	if(nbr) {
		//if neighbor exists, update dead time
		gettimeofday(&nbr->last_heard,NULL);
		add_event((struct replay_object*)nbr,OSPF_EVENT_NBR_DEAD);
	} else {
		//else create new neighbor
		nbr = (struct ospf_neighbor *)malloc(sizeof(struct ospf_neighbor));
		nbr->dead_interval = dead_interval;
		nbr->hello_interval = hello_interval;
		nbr->ip.s_addr = ip;
		nbr->options = options;
		nbr->ospf_if = ospf_if;
		nbr->priority = priority;
		nbr->router_id.s_addr = router_id;
		nbr->state = OSPF_NBRSTATE_DOWN;
		gettimeofday(&nbr->last_heard,NULL);
		add_event((struct replay_object*)nbr,OSPF_EVENT_NBR_DEAD);

		new_item = (struct replay_list *)malloc(sizeof(struct replay_list));
		new_item->next = NULL;
		new_item->object = (struct replay_object *)nbr;

		ospf0->nbrlist = add_to_list(ospf0->nbrlist,new_item);
		ospf0->nbrcount++;

		new_item = (struct replay_list *)malloc(sizeof(struct replay_list));
		new_item->next = NULL;
		new_item->object = (struct replay_object *)nbr;
		ospf_if->nbrlist = add_to_list(ospf_if->nbrlist,new_item);

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
