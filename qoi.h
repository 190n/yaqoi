#ifndef __QOI_H__
#define __QOI_H__

#include <stdint.h>

// "qoif"
#define QOI_MAGIC (('q' << 24) | ('o' << 16) | ('i' << 8) | ('f' << 0))

#define RGB 3
#define RGBA 4
#define QOI_SRGB 0
#define QOI_LINEAR 1

typedef uint8_t qoi_channels_t;
typedef uint8_t qoi_colorspace_t;

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

#define QOI_HEADER_LENGTH 14

#endif
