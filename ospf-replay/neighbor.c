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

struct ospf_neighbor* add_neighbor(u_int32_t ip,u_int32_t router_id,struct ospf_interface *ospf_if) {
	return NULL;
}

void remove_neighbor(struct ospf_neighbor *ospf_neigh) {

}
