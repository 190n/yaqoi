#ifndef __BITS_H__
#define __BITS_H__

#include <stdint.h>

//
// Store a 32-bit unsigned integer in memory in big-endian order.
//
// dest: where to store the bytes
// x:    number to store
//
static inline void store_u32be(uint8_t dest[4], uint32_t x) {
	dest[0] = (x & 0xff000000) >> 24;
	dest[1] = (x & 0x00ff0000) >> 16;
	dest[2] = (x & 0x0000ff00) >> 8;
	dest[3] = (x & 0x000000ff) >> 0;
}

#endif
