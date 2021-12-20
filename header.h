#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdint.h>

enum qoi_channels_t {
	RGB = 3,
	RGBA = 4,
};

enum qoi_colorspace_t {
	QOI_SRGB = 0,
	QOI_LINEAR = 1,
};

typedef struct {
	char magic[4];
	uint32_t width;
	uint32_t height;
	qoi_channels_t channels;
	qoi_colorspace_t colorspace;
} qoi_header_t;

#endif
