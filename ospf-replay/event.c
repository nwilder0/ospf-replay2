/*
 * event.c
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

void check_events() {
	struct timeval now;
	struct replay_nlist *tmp_item,*prev_item;
	gettimeofday(&now,NULL);
	tmp_item = ospf0->eventlist;
	while(tmp_item) {
		if(tmp_item->key <= now.tv_sec) {
			prev_item = tmp_item;
			tmp_item = tmp_item->next;
			do_event(prev_item);
		}
		else {
			tmp_item = NULL;
		}
	}
}

void do_event(struct replay_nlist *item) {
	struct ospf_event *event;
	event = (struct ospf_event *) item->object;

	switch(event->type) {

		case OSPF_EVENT_LSA_AGING:
			remove_lsa((struct ospf_lsa *)event->object);
			break;
	}

	ospf0->eventlist = remove_from_nlist(ospf0->eventlist,item);
	free(event);
}

void add_event(struct replay_object *object,u_int8_t type) {
	struct ospf_event *new,*tmp;
	struct replay_nlist *new_item;

	if(object) {

		new = (struct ospf_event *) malloc(sizeof(struct ospf_event));
		gettimeofday(&new->tv,NULL);
		new->object = object;
		new->type = type;

		switch(type) {

		case OSPF_EVENT_LSA_AGING:
			new->tv.tv_sec = new->tv.tv_sec + OSPF_LSA_MAX_AGE;
			tmp = find_event(object,type);
			if(tmp) {
				remove_event(tmp);
			}
			break;
		}

		new_item = (struct replay_nlist *) malloc(sizeof(struct replay_nlist));
		new_item->key = (unsigned long long)new->tv.tv_sec;
		new_item->next = NULL;
		new_item->object = (struct replay_object *)new;
		ospf0->eventlist = add_to_nlist(ospf0->eventlist,new_item);
	}
}

void remove_event(struct ospf_event *remove) {
	struct replay_nlist *item;
	if(remove) {
		item = find_in_nlist(ospf0->eventlist,(struct replay_object *)remove);
		if(item) {
			ospf0->eventlist = remove_from_nlist(ospf0->eventlist,item);
		}
	}
}

struct ospf_event* find_event(struct replay_object *object, u_int8_t type) {
	struct ospf_event *found,*tmp;
	struct replay_nlist *curr;
	found = NULL;
	curr = ospf0->eventlist;
	while(curr && !found) {
		tmp = (struct ospf_event *)curr->object;
		if(tmp) {
			if((tmp->object == object) && (tmp->type == type)) {
				found = tmp;
			}
		}
		curr = curr->next;
	}
	return found;
}
