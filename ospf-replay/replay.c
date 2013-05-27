/*
 * replay.c
 *
 *  Created on: May 19, 2013
 *      Author: nathan
 */

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
#include "replay.h"

void print_packet(unsigned char*, int);
void print_mesg_hello(unsigned char*, int);
void hello_reply(unsigned char*, int);

void error(char* mesg) {
	printf("%s\n", mesg);
	exit(1);
}

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

int ospf_socket;

int main()
{

	struct ifreq *ifr;

	printf("hello world\n");

	int bool=0;
	int n,fromlen;
	char tmpstr_ipv4[INET_ADDRSTRLEN];
	unsigned char *buffer = (unsigned char *)malloc(65536);
	struct sockaddr_in serv_addr, peer_addr;

	ospf_socket = socket(AF_INET,SOCK_RAW,89);
	if(ospf_socket<0) {
		error("Error opening socket");
	}
	printf("socket opened\n");

	bzero((char *) &peer_addr, sizeof(peer_addr));
	bzero((char *) &serv_addr, sizeof(serv_addr));


	peer_addr.sin_family = AF_INET;
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET,"172.16.2.2",&serv_addr.sin_addr);
	//serv_addr.sin_addr.s_addr = inet_ntop("172.16.2.2");
	if (setsockopt(ospf_socket, SOL_SOCKET, SO_BINDTODEVICE, &ifr[3], sizeof(ifr[3])) < 0)
		error("ERROR on interface binding");
//	if (bind(ospf_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
//	  error("ERROR on binding");

	struct ip_mreq mreq;
	inet_pton(AF_INET,"172.16.2.2",&mreq.imr_interface);
	inet_pton(AF_INET,"224.0.0.5",&mreq.imr_multiaddr);
	//mreq.imr_multiaddr.s_addr = inet_ntop("224.0.0.5");
	if(setsockopt(ospf_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))<0)
		error("ERROR on multicast join");

	printf("about to read\n");

	//n = read(ospf_socket, buffer, 255);
	fromlen = sizeof(peer_addr);
	while(1) {
	n = recvfrom(ospf_socket, buffer, 4096, 0, &peer_addr, &fromlen);
	printf("after reading\n");
	if (n<0) error("Error reading from socket");

	inet_ntop(AF_INET,&peer_addr.sin_addr,tmpstr_ipv4,INET_ADDRSTRLEN);

	if(strcmp(tmpstr_ipv4,"172.16.2.2")) {
		print_packet(buffer,n);
	}

	printf("\n");
	}
	close(ospf_socket);
	return 0;
}

void print_packet(unsigned char* buffer, int size) {

	struct iphdr *ipheader = (struct iphdr *)buffer;
	struct sockaddr_in src_addr, dest_addr, src_rtr, area_id;
	unsigned int iphdrlen;
	char tmpstr_ipv4[INET_ADDRSTRLEN];
	int i,bool=0;

	iphdrlen = ipheader->ihl*4;

	bzero((char *) &src_addr, sizeof(src_addr));
	bzero((char *) &dest_addr, sizeof(dest_addr));

	src_addr.sin_addr.s_addr = ipheader->saddr;
	dest_addr.sin_addr.s_addr = ipheader->daddr;

	printf("\nHex of packet follows:%s.\nsizeof:%i\n",buffer,size);
	for(i=0;i<size;i++) {

		printf("%02x",buffer[i]);
		if(bool)
			printf(" ");
		bool = abs(bool-1);

	}

	printf("\n");
	printf("IP Header\n");
	printf("   |-IP Version        : %d\n",(unsigned int)ipheader->version);
	printf("   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)ipheader->ihl,((unsigned int)(ipheader->ihl))*4);
	printf("   |-Type Of Service   : %d\n",(unsigned int)ipheader->tos);
	printf("   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(ipheader->tot_len));
	printf("   |-Identification    : %d\n",ntohs(ipheader->id));
	printf("   |-TTL      : %d\n",(unsigned int)ipheader->ttl);
	printf("   |-Protocol : %d\n",(unsigned int)ipheader->protocol);
	printf("   |-Checksum : %d\n",ntohs(ipheader->check));
	inet_ntop(AF_INET,&src_addr.sin_addr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Source IP        : %s\n",tmpstr_ipv4);
	inet_ntop(AF_INET,&dest_addr.sin_addr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Destination IP   : %s\n",tmpstr_ipv4);

	struct ospfhdr *ospfheader=(struct ospfhr*)(buffer + iphdrlen);

	bzero((char *) &src_rtr, sizeof(src_rtr));
	bzero((char *) &area_id, sizeof(area_id));

	src_rtr.sin_addr.s_addr = ospfheader->src_router;
	area_id.sin_addr.s_addr = ospfheader->area_id;

	printf("\n");
	printf("OSPF Header\n");
	printf("   |-OSPF Version   : %d\n",(unsigned int)ospfheader->version);
	printf("   |-Packet Length  : %d Bytes\n",(unsigned int)ntohs(ospfheader->packet_length));
	printf("   |-Message Type   : %d\n",(unsigned int)ospfheader->mesg_type);
	printf("   |-Checksum       : %04x\n",ntohs(ospfheader->checksum));
	inet_ntop(AF_INET,&src_rtr.sin_addr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Source Router  : %s\n",tmpstr_ipv4);
	inet_ntop(AF_INET,&area_id.sin_addr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Area ID        : %s\n",tmpstr_ipv4);

	switch(ospfheader->mesg_type)
	{
		case 1:
			print_mesg_hello(buffer+iphdrlen+OSPFHDR_LEN,size-iphdrlen-OSPFHDR_LEN);
			hello_reply(buffer,size);
		default:
			break;
	}

}

void print_mesg_hello(unsigned char* mesg, int size) {

	struct ospf_hello *hello_mesg = (struct ospf_hello *)mesg;
	struct sockaddr_in dr, bdr, netmask;
	char tmpstr_ipv4[INET_ADDRSTRLEN];

	bzero((char *) &dr, sizeof(dr));
	bzero((char *) &bdr, sizeof(bdr));
	bzero((char *) &netmask, sizeof(netmask));

	dr.sin_addr = hello_mesg->dr;
	bdr.sin_addr = hello_mesg->bdr;
	netmask.sin_addr = hello_mesg->network_mask;

	printf("\n");
	printf("OSPF Message\n");
	inet_ntop(AF_INET,&hello_mesg->network_mask,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Network Mask       : %s\n",tmpstr_ipv4);
	printf("   |-Hello Interval     : %d seconds\n",ntohs(hello_mesg->hello_interval));
	printf("   |-Options            : %s\n",byte_to_binary((unsigned int)hello_mesg->options));
	printf("   |-Priority           : %d\n",(unsigned int)hello_mesg->priority);
	printf("   |-Dead Interval      : %d seconds\n",ntohl(hello_mesg->dead_interval));
	inet_ntop(AF_INET,&hello_mesg->dr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Designated Router  : %s\n",tmpstr_ipv4);
	inet_ntop(AF_INET,&hello_mesg->bdr,tmpstr_ipv4,INET_ADDRSTRLEN);
	printf("   |-Backup DR          : %s\n",tmpstr_ipv4);


}

void hello_reply(unsigned char* buffer, int size) {

	struct iphdr *ipheader = (struct iphdr *)buffer;
	struct sockaddr_in dest_addr;
	unsigned int iphdrlen;
	int n;

	iphdrlen = ipheader->ihl*4;

	struct ospfhdr *new_ospfheader=(struct ospfhr*)(buffer + iphdrlen);

	struct ospf_hello *new_hello_mesg = (struct ospf_hello *)(buffer + iphdrlen + OSPFHDR_LEN);

	unsigned char new_mesg[size-iphdrlen+4];

	inet_pton(AF_INET,"172.16.2.2",&new_ospfheader->src_router);
	inet_pton(AF_INET,"172.16.2.2",&new_hello_mesg->dr);
	//new_ospfheader->checksum = htons(19856);
	//if(new_hello_mesg->neighbors[0].s_addr==new_ospfheader->src_router) {
		inet_pton(AF_INET,"192.168.255.2",&new_hello_mesg->neighbors[0].s_addr);
		new_ospfheader->checksum = htons(57293);
		new_ospfheader->packet_length = htons(48);
	//}
	memcpy(new_mesg,new_ospfheader,OSPFHDR_LEN);
	memcpy(new_mesg+OSPFHDR_LEN,new_hello_mesg,48);

	bzero((char *) &dest_addr, sizeof(dest_addr));

	dest_addr.sin_family = AF_INET;
	inet_pton(AF_INET,"224.0.0.5",&dest_addr.sin_addr);

	n = sendto(ospf_socket, new_mesg, 48, 0, &dest_addr, sizeof(dest_addr));
	if(n<1)
		printf("\nERROR sending reply\n");
}
