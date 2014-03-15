/*
 * lsa.h
 *
 *  Created on: Jun 15, 2013
 *      Author: nathan
 */


#ifndef LSA_H_
#define LSA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "event.h"
#include "interface.h"
#include "load.h"
#include "neighbor.h"
#include "prefix.h"
#include "utility.h"
#include "replay.h"
#include "packet.h"

/* OSPF LSA Range definition. */
#define OSPF_MIN_LSA		1  /* begin range here */
#define OSPF_MAX_LSA		8

/* OSPF LSA Type definition. */
#define OSPF_LSATYPE_UNKNOWN	          0
#define OSPF_LSATYPE_ROUTER               1
#define OSPF_LSATYPE_NETWORK              2
#define OSPF_LSATYPE_SUMMARY              3
#define OSPF_LSATYPE_ASBR_SUMMARY         4
#define OSPF_LSATYPE_AS_EXTERNAL          5
#define OSPF_LSATYPE_GROUP_MEMBER         6  /* Not supported. */
#define OSPF_LSATYPE_AS_NSSA              7
#define OSPF_LSATYPE_EXTERNAL_ATTRIBUTES  8  /* Not supported. */

#define OSPF_LSA_HEADER_SIZE	     20U
#define OSPF_ROUTER_LSA_LINK_SIZE    12U
#define OSPF_ROUTER_LSA_TOS_SIZE      4U
#define OSPF_MAX_LSA_SIZE	   1500U

/* AS-external-LSA refresh method. */
#define LSA_REFRESH_IF_CHANGED	0
#define LSA_REFRESH_FORCE	1

/* OSPF LSA header. */
struct lsa_header
{
  u_int16_t ls_age;
  u_char options;
  u_char type;
  struct in_addr id;
  struct in_addr adv_router;
  u_int32_t ls_seqnum;
  u_int16_t checksum;
  u_int16_t length;
};

/* OSPF LSA. */
struct ospf_lsa
{
  /* LSA origination flag. */
  u_char flags;
#define OSPF_LSA_SELF		  0x01
#define OSPF_LSA_SELF_CHECKED	  0x02
#define OSPF_LSA_RECEIVED	  0x04
#define OSPF_LSA_APPROVED	  0x08
#define OSPF_LSA_DISCARD	  0x10
#define OSPF_LSA_LOCAL_XLT	  0x20
#define OSPF_LSA_PREMATURE_AGE	  0x40
#define OSPF_LSA_IN_MAXAGE	  0x80

  /* LSA data. */
  struct lsa_header *header;

  /* Received time stamp. */
  struct timeval tv_recv;

  /* Last time it was originated */
  struct timeval tv_orig;

  /* All of reference count, also lock to remove. */
  int lock;

  /* Flags for the SPF calculation. */
  int stat;
  #define LSA_SPF_NOT_EXPLORED	-1
  #define LSA_SPF_IN_SPFTREE	-2
  /* If stat >= 0, stat is LSA position in candidates heap. */

  /* References to this LSA in neighbor retransmission lists*/
  int retransmit_counter;

  /* Area the LSA belongs to, may be NULL if AS-external-LSA. */
  struct ospf_area *area;

  /* Parent LSDB. */
  struct ospf_lsdb *lsdb;

  /* Related Route. */
  void *route;

  struct in_addr ip;

  unsigned long long hdr_ptr;

  /* Refreshement List or Queue */
  int refresh_list;

  /* For Type-9 Opaque-LSAs */
  struct ospf_interface *oi;

  u_int8_t virtual;

};

/* OSPF LSA Link Type. */
#define LSA_LINK_TYPE_POINTOPOINT      1
#define LSA_LINK_TYPE_TRANSIT          2
#define LSA_LINK_TYPE_STUB             3
#define LSA_LINK_TYPE_VIRTUALLINK      4

/* OSPF Router LSA Flag. */
#define ROUTER_LSA_BORDER	       0x01 /* The router is an ABR */
#define ROUTER_LSA_EXTERNAL	       0x02 /* The router is an ASBR */
#define ROUTER_LSA_VIRTUAL	       0x04 /* The router has a VL in this area */
#define ROUTER_LSA_NT		       0x10 /* The routers always translates Type-7 */
#define ROUTER_LSA_SHORTCUT	       0x20 /* Shortcut-ABR specific flag */

#define IS_ROUTER_LSA_VIRTUAL(x)       ((x)->flags & ROUTER_LSA_VIRTUAL)
#define IS_ROUTER_LSA_EXTERNAL(x)      ((x)->flags & ROUTER_LSA_EXTERNAL)
#define IS_ROUTER_LSA_BORDER(x)	       ((x)->flags & ROUTER_LSA_BORDER)
#define IS_ROUTER_LSA_SHORTCUT(x)      ((x)->flags & ROUTER_LSA_SHORTCUT)
#define IS_ROUTER_LSA_NT(x)            ((x)->flags & ROUTER_LSA_NT)

/* OSPF Router-LSA Link information. */
struct router_lsa_link
{
  struct in_addr link_id;
  struct in_addr link_data;
  u_char type;
  u_char tos;
  u_int16_t metric;

};

/* OSPF Router-LSAs structure. */
#define OSPF_ROUTER_LSA_MIN_SIZE                   4U /* w/0 link descriptors */
/* There is an edge case, when number of links in a Router-LSA may be 0 without
   breaking the specification. A router, which has no other links to backbone
   area besides one virtual link, will not put any VL descriptor blocks into
   the Router-LSA generated for area 0 until a full adjacency over the VL is
   reached (RFC2328 12.4.1.3). In this case the Router-LSA initially received
   by the other end of the VL will have 0 link descriptor blocks, but soon will
   be replaced with the next revision having 1 descriptor block. */
struct router_lsa
{
  struct lsa_header header;
  u_char flags;
  u_char zero;
  u_int16_t links;
  struct router_lsa_link link[1];
};


/* OSPF Network-LSAs structure. */
#define OSPF_NETWORK_LSA_MIN_SIZE                  8U /* w/1 router-ID */
struct network_lsa
{
  struct lsa_header header;
  struct in_addr mask;
  struct in_addr routers[1];
};

/* OSPF Summary-LSAs structure. */
#define OSPF_SUMMARY_LSA_MIN_SIZE                  8U /* w/1 TOS metric block */
struct summary_lsa
{
  struct lsa_header header;
  struct in_addr mask;
  u_char tos;
  u_char metric[3];
};

/* OSPF AS-external-LSAs structure. */
#define OSPF_AS_EXTERNAL_LSA_MIN_SIZE             16U /* w/1 TOS forwarding block */
struct as_external_lsa
{
  struct lsa_header header;
  struct in_addr mask;
  struct
  {
    u_char tos;
    u_char metric[3];
    struct in_addr fwd_addr;
    u_int32_t route_tag;
  } e[1];
};

struct lsa {
	void* lsa;
	u_char type;
};

#define OSPF_LSA_INITIAL_AGE 0
#define OSPF_INITIAL_SEQUENCE_NUMBER 1
#define OSPF_LSA_MAX_AGE 3600
#define OSPF_LSA_RESEND_SELF 1800

struct router_lsa* set_router_lsa();
u_int16_t ospf_lsa_checksum (struct lsa_header *);
int ospf_lsa_checksum_valid (struct lsa_header *);

struct ospf_lsa* add_lsa(struct lsa_header *);
void remove_lsa(struct ospf_lsa *);
struct replay_list* copy_lsalist();
int have_lsa(struct lsa_header *);
struct ospf_lsa* find_lsa(u_int32_t,u_int32_t,u_char);

#endif /* LSA_H_ */
