#ifndef __PIXEL_H__
#define __PIXEL_H__

#include <stdint.h>

typedef struct {
	uint8_t r, g, b, a;
} pixel_t;

static inline uint8_t pixel_hash(pixel_t p);

#endif
