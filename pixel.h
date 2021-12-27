#ifndef __PIXEL_H__
#define __PIXEL_H__

#include <stdbool.h>
#include <stdint.h>

typedef union {
	struct {
		uint8_t r, g, b, a;
	} channels;
	uint32_t rgba;
} pixel_t;

typedef struct {
	uint8_t r, g, b, a;
} pixel_difference_t;

//
// Compute the hash of a pixel, to determine its index in the array of seen pixels.
//
// p: the pixel to hash
//
static inline uint8_t pixel_hash(pixel_t p) {
	return (p.channels.r * 3 + p.channels.g * 5 + p.channels.b * 7 + p.channels.a * 11) % 64;
}

//
// Determine if two pixels are equal.
//
// a: the first pixel
// b: the second pixel
//
static inline bool pixel_equal(pixel_t a, pixel_t b) {
	return a.rgba == b.rgba;
}

//
// Compute the difference between two pixels (a - b).
//
// a: the first pixel
// b: the second pixel
//
static inline pixel_difference_t pixel_subtract(pixel_t a, pixel_t b) {
	return (pixel_difference_t) {
		.r = a.channels.r - b.channels.r,
		.g = a.channels.g - b.channels.g,
		.b = a.channels.b - b.channels.b,
		.a = a.channels.a - b.channels.a,
	};
}

#endif
