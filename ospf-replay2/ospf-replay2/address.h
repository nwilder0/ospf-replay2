/*
 * address.h
 *
 *  Created on: Mar 15, 2014
 *      Author: nathan
 */

#ifndef ADDRESS_H_
#define ADDRESS_H_
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

class address {
public:
	address();
	virtual ~address();

	virtual int getBits();
	virtual long getAddr();
	virtual long getMask();
	virtual long getNet();
	virtual long getBCast();

private:

};

class ipv4 : address {
public:
	ipv4();
	ipv4(uint32_t,uint32_t);
	virtual ~ipv4();

	int getBits();
	uint32_t getAddr();
	uint32_t getMask();
	uint32_t getNet();
	uint32_t getBCast();
	static uint32_t bits2mask(int);
	static int mask2bits(uint32_t);
	static uint32_t getNet(uint32_t, uint32_t);
	static bool ipInNet(u_int32_t ip, u_int32_t net, u_int32_t mask);

private:
	uint32_t addr;
	uint32_t mask;
	uint8_t bits;
	uint32_t net;
	uint32_t bcast;
};

#endif /* ADDRESS_H_ */
