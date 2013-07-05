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
	nbr = find_neighbor_by_ip(ip);
	if(nbr) {
		//if neighbor exists, update dead time

	} else {
		//else create new neighbor
	}
}

void remove_neighbor(struct ospf_neighbor *ospf_neigh) {

}

struct ospf_neighbor* find_neighbor_by_ip(u_int32_t ip) {
	return NULL;
}
