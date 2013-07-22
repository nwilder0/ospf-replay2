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
	struct ospf_interface *oiface;
	event = (struct ospf_event *) item->object;

	switch(event->type) {

	case OSPF_EVENT_HELLO_BROADCAST:
		send_hello((struct ospf_interface *)event->object,NULL);
		add_event(event->object,OSPF_EVENT_HELLO_BROADCAST);
		break;

	case OSPF_EVENT_LSA_AGING:
		//send out max aged lsa
		remove_lsa((struct ospf_lsa *)event->object);
		break;

	case OSPF_EVENT_NBR_DEAD:
		remove_neighbor((struct ospf_neighbor *)event->object);
		break;

	case OSPF_EVENT_NO_DR:
		oiface = (struct ospf_interface *)event->object;
		if(!oiface->dr.s_addr) {
			oiface->dr.s_addr = ospf0->router_id.s_addr;
		}
		break;

	case OSPF_EVENT_DBDESC_RETX:
		send_dbdesc((struct ospf_neighbor *)(event->object),0);
		break;

	case OSPF_EVENT_LSR_RETX:
		send_lsr((struct ospf_neighbor *)(event->object));
		break;

	case OSPF_EVENT_LSU_ACK:
		send_lsu((struct ospf_neighbor *)(event->object),NULL,OSPF_LSU_RETX);
		break;

	case OSPF_EVENT_LSA_REFRESH:
		set_router_lsa();
		break;
	}

	ospf0->eventlist = remove_from_nlist(ospf0->eventlist,item);
	free(event);
}

void add_event(void *object,u_int8_t type) {
	struct ospf_event *new,*tmp;

	if(object) {

		new = malloc(sizeof(*new));
		memset(new,0,sizeof(*new));
		gettimeofday(&new->tv,NULL);
		new->object = object;
		new->type = type;

		switch(type) {

		case OSPF_EVENT_HELLO_BROADCAST:
			new->tv.tv_sec = new->tv.tv_sec + ospf0->hello_interval;
			break;

		case OSPF_EVENT_LSA_AGING:
			new->tv.tv_sec = ((struct ospf_lsa *)object)->tv_orig.tv_sec + OSPF_LSA_MAX_AGE;
			tmp = find_event(object,type);
			if(tmp) {
				remove_event(tmp);
			}
			break;

		case OSPF_EVENT_NBR_DEAD:
			new->tv.tv_sec = new->tv.tv_sec + ospf0->dead_interval;
			tmp = find_event(object,type);
			if(tmp) {
				remove_event(tmp);
			}
			break;

		case OSPF_EVENT_NO_DR:
			new->tv.tv_sec = new->tv.tv_sec + OSPF_WAIT_FOR_DR;
			break;

		case OSPF_EVENT_DBDESC_RETX:
		case OSPF_EVENT_LSR_RETX:
		case OSPF_EVENT_LSU_ACK:
			new->tv.tv_sec = new->tv.tv_sec + ospf0->retransmit_interval;
			break;

		case OSPF_EVENT_LSA_REFRESH:
			new->tv.tv_sec = new->tv.tv_sec + OSPF_LSA_RESEND_SELF;
			break;

		}

		ospf0->eventlist = add_to_nlist(ospf0->eventlist,(void *)new,(unsigned long long)new->tv.tv_sec);
	}
}

void remove_event(struct ospf_event *remove) {
	struct replay_nlist *item;
	if(remove) {
		item = find_in_nlist(ospf0->eventlist,(void *)remove);
		if(item) {
			ospf0->eventlist = remove_from_nlist(ospf0->eventlist,item);
		}
	}
}

struct ospf_event* find_event(void *object, u_int8_t type) {
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

void remove_object_events(void *object) {

	struct ospf_event *tmp;
	struct replay_nlist *curr;

	curr = ospf0->eventlist;
	while(curr) {
		tmp = (struct ospf_event *)curr->object;
		if(tmp) {
			if(tmp->object == object) {
				remove_event(tmp);
			}
		}
		curr = curr->next;
	}

}

void find_and_remove_event(void *obj, u_int8_t type) {
	struct ospf_event *found;
	found = find_event(obj,type);
	if(found) {
		remove_event(found);
	}
}
