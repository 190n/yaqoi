#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "pixel.h"
#include "qoi.h"
#include "stats.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct Encoder Encoder;

Encoder *encoder_create(bool track_stats, qoi_desc_t *desc);

void encoder_write_header(FILE *dest, Encoder *e);

void encoder_encode_pixels(FILE *dest, Encoder *e, pixel_t *pixels, uint64_t n);

void encoder_finish(FILE *dest, Encoder *e);

void encoder_flush(FILE *dest, Encoder *e);

qoi_stats_t *encoder_get_stats(Encoder *e);

void encoder_delete(Encoder **e);

#endif
