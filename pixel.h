#ifndef __PIXEL_H__
#define __PIXEL_H__

#include <stdint.h>

typedef struct {
	uint8_t r, g, b, a;
} pixel_t;

//
// Compute the hash of a pixel, to determine its index in the array of seen pixels.
//
// p: the pixel to hash
//
static inline uint8_t pixel_hash(pixel_t p) {
	return (p.r * 3 + p.g * 5 + p.b * 7 + p.a * 11) % 64;
}

#endif
