#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "qoi.h"
#include "pixel.h"
#include "stats.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct Encoder Encoder;

Encoder *encoder_create(const char *output, bool track_stats, qoi_desc_t *desc);

void encoder_write_header(Encoder *e);

void encoder_encode_pixels(Encoder *e, pixel_t *pixels, uint64_t n);

void encoder_finish(Encoder *e);

void encoder_flush(Encoder *e);

qoi_stats_t encoder_get_stats(Encoder *e);

void encoder_delete(Encoder **e);

#endif
