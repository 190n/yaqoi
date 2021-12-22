#ifndef __QOI_H__
#define __QOI_H__

#include <stdint.h>

typedef enum {
	RGB = 3,
	RGBA = 4,
} qoi_channels_t;

typedef enum {
	QOI_SRGB = 0,
	QOI_LINEAR = 1,
} qoi_colorspace_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	qoi_channels_t channels;
	qoi_colorspace_t colorspace;
} qoi_desc_t;

typedef struct {
	char magic[4];
	uint32_t width;
	uint32_t height;
	qoi_channels_t channels;
	qoi_colorspace_t colorspace;
} qoi_header_t;

#endif
