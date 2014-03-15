/*
 * address.cpp
 *
 *  Created on: Mar 15, 2014
 *      Author: nathan
 */

#include "address.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

address::address() {
	// TODO Auto-generated constructor stub

}

address::~address() {
	// TODO Auto-generated destructor stub
}


ipv4::ipv4() {
	// TODO Auto-generated constructor stub

}

ipv4::ipv4(uint32_t ip,uint32_t mask) {
	// TODO Auto-generated constructor stub
	addr = ip;
	this->mask = mask;
	bits = mask2bits(this->mask);
}

ipv4::~ipv4() {
	// TODO Auto-generated destructor stub
}

static uint32_t ipv4::bits2mask(int bits) {

	uint32_t mask = 0;

	mask = pow(2,bits)-1;

	mask = mask << (32-bits);

	mask = htonl(mask);

	return mask;
}

static int ipv4::mask2bits(uint32_t mask) {

	int bits = 0;

	for(bits = 0; mask; mask >>= 1)
	{
	  bits += mask & 1;
	}

	return bits;
}

static uint32_t ipv4::getNet(uint32_t addr, uint32_t mask) {

	uint32_t net_addr;

	net_addr = addr & mask;

	return net_addr;
}

static bool ipv4::ipInNet(u_int32_t ip, u_int32_t net, u_int32_t mask) {
	bool bFound=0;
	u_int32_t cmp_net=0;

	cmp_net = ipv4::getNet(ip,mask);
	if(cmp_net == net) {
		bFound = 1;
	}

	return bFound;
}
