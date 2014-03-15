/*
 * checksum.h
 *
 *  Created on: Jun 15, 2013
 *      Author: nathan
 */

#ifndef CHECKSUM_H_
#define CHECKSUM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/param.h>

extern int in_cksum(void *, int);
#define FLETCHER_CHECKSUM_VALIDATE 0xffff
extern u_int16_t fletcher_checksum(u_char *, const size_t len, const uint16_t offset);


#endif /* CHECKSUM_H_ */
