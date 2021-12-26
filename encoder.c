#include "encoder.h"

#include "bits.h"
#include "pixel.h"

#include <stdlib.h>
#include <string.h>

struct Encoder {
	qoi_stats_t stats;
	qoi_desc_t desc;
	pixel_t seen_pixels[64];
	pixel_t last_pixel;
	uint8_t run_length;
	uint64_t total_pixels;
	bool track_stats;
};

//
// Create a new QOI encoder.
//
// track_stats: whether to track statistics during encoding
// desc:        information about the file to encode
//
Encoder *encoder_create(bool track_stats, qoi_desc_t *desc) {
	// use calloc for zero initialization
	Encoder *e = (Encoder *) calloc(1, sizeof(Encoder));
	if (e) {
		e->track_stats = track_stats;
		e->desc = *desc;
		e->total_pixels = ((uint64_t) desc->width) * ((uint64_t) desc->height);
		e->last_pixel.channels.a = 255;
	}

	return e;
}

//
// Write the header of a QOI file.
//
// dest: file to write the header to
// e:    QOI encoder to use
//
void encoder_write_header(FILE *dest, Encoder *e) {
	if (e) {
		qoi_header_t h;
		store_u32be(h.magic, QOI_MAGIC);
		store_u32be((uint8_t *) &h.width, e->desc.width);
		store_u32be((uint8_t *) &h.height, e->desc.height);
		h.channels = e->desc.channels;
		h.colorspace = e->desc.colorspace;
		fwrite(&h, QOI_HEADER_LENGTH, 1, dest);
	}
}

//
// Write a QOI_OP_RUN chunk representing an encoder's current run to a file, update statistics, and
// end the run in the encoder's state.
//
// dest: file to write to
// e:    QOI encoder to use
//
void write_run(FILE *dest, Encoder *e) {
	uint8_t chunk = QOI_OP_RUN;
	chunk |= (e->run_length - 1);
	fputc(chunk, dest);

	if (e->track_stats) {
		e->stats.total_bits += 8;
		e->stats.total_pixels += e->run_length;
		e->stats.op_to_pixels.run += e->run_length;
	}

	e->run_length = 0;
}

//
// Write a QOI_OP_INDEX chunk to a file and update the encoder's statistics.
//
// dest:  file to write to
// e:     QOI encoder to use
// index: index to write
//
void write_index(FILE *dest, Encoder *e, uint8_t index) {
	uint8_t chunk = QOI_OP_INDEX;
	chunk |= index;
	fputc(chunk, dest);

	if (e->track_stats) {
		e->stats.total_bits += 8;
		e->stats.total_pixels += 1;
		e->stats.op_to_pixels.index += 1;
	}
}

//
// Determine if the difference between two pixels is suitable for a QOI_OP_DIFF chunk (RGB
// difference is in [-2, 1] for each channel, and alpha channels are equal).
//
// diff: pixel difference to examine
//
bool op_diff_compatible(pixel_difference_t *diff) {
	return (-2 <= diff->r && diff->r <= 1) && (-2 <= diff->g && diff->g <= 1)
	       && (-2 <= diff->b && diff->b <= 1) && (diff->a == 0);
}

//
// Write a QOI_OP_DIFF chunk to a file and update the encoder's statistics.
//
// dest: file to write to
// e:    QOI encoder to use
// diff: pixel difference to write
//
void write_diff(FILE *dest, Encoder *e, pixel_difference_t *diff) {
	uint8_t chunk = QOI_OP_DIFF, dr = diff->r + 2, dg = diff->g + 2, db = diff->b + 2;
	chunk |= (dr << 4);
	chunk |= (dg << 2);
	chunk |= (db << 0);
	fputc(chunk, dest);

	if (e->track_stats) {
		e->stats.total_bits += 8;
		e->stats.total_pixels += 1;
		e->stats.op_to_pixels.diff += 1;
	}
}

//
// Determine if the difference between two pixels is suitable for a QOI_OP_LUMA chunk:
//  - green channel difference is in [-32, 31]
//  - red and blue differences minus the green channel difference are in [-8, 7]
//  - alpha channels are equal
//
// diff: pixel difference to examine
//
bool op_luma_compatible(pixel_difference_t *diff) {
	int16_t dr_dg = diff->r - diff->g, db_dg = diff->b - diff->g;
	return (-32 <= diff->g && diff->g <= 31) && (-8 <= dr_dg && dr_dg <= 7)
	       && (-8 <= db_dg && db_dg <= 7) && (diff->a == 0);
}

//
// Write a QOI_OP_LUMA chunk to a file and update the encoder's statistics.
//
// dest: file to write to
// e:    QOI encoder to use
// diff: pixel difference to write
//
void write_luma(FILE *dest, Encoder *e, pixel_difference_t *diff) {
	uint8_t chunk[2];
	uint8_t dg = diff->g + 32, dr_dg = diff->r - dg + 8, db_dg = diff->b - dg + 8;
	chunk[0] = QOI_OP_LUMA | dg;
	chunk[1] = (dr_dg << 4) | (db_dg << 0);
	fwrite(chunk, sizeof(uint8_t), 2, dest);

	if (e->track_stats) {
		e->stats.total_bits += 16;
		e->stats.total_pixels += 1;
		e->stats.op_to_pixels.luma += 1;
	}
}

//
// Write a QOI_OP_RGBA chunk to a file and update the encoder's statistics.
//
// dest: file to write to
// e:    QOI encoder to use
// p:    pixel to write
//
void write_rgba(FILE *dest, Encoder *e, pixel_t *p) {
	uint8_t chunk[5];
	chunk[0] = QOI_OP_RGBA;
	chunk[1] = p->channels.r;
	chunk[2] = p->channels.g;
	chunk[3] = p->channels.b;
	chunk[4] = p->channels.a;
	fwrite(chunk, sizeof(uint8_t), 5, dest);

	if (e->track_stats) {
		e->stats.total_bits += 40;
		e->stats.total_pixels += 1;
		e->stats.op_to_pixels.rgba += 1;
	}
}

//
// Write a QOI_OP_RGB chunk to a file and update the encoder's statistics.
//
// dest: file to write to
// e:    QOI encoder to use
// p:    pixel to write
//
void write_rgb(FILE *dest, Encoder *e, pixel_t *p) {
	uint8_t chunk[4];
	chunk[0] = QOI_OP_RGB;
	chunk[1] = p->channels.r;
	chunk[2] = p->channels.g;
	chunk[3] = p->channels.b;
	fwrite(chunk, sizeof(uint8_t), 4, dest);

	if (e->track_stats) {
		e->stats.total_bits += 32;
		e->stats.total_pixels += 1;
		e->stats.op_to_pixels.rgb += 1;
	}
}

//
// Encode some pixels using QOI.
//
// dest:   file to write encoded data to
// e:      QOI encoder to use
// pixels: array of pixels to encode
// n:      number of pixels to encode
//
void encoder_encode_pixels(FILE *dest, Encoder *e, pixel_t *pixels, uint64_t n) {
	for (uint64_t i = 0; i < n; i++) {
		pixel_t p = pixels[i];
		if (pixel_equal(p, e->last_pixel)) {
			// this pixel is the same as the last one, so encode a run
			e->run_length++;
			if (e->run_length == 62) {
				// longest that a run can be
				write_run(dest, e);
			}

			// pixel is now handled
			continue;
		} else if (e->run_length > 0) {
			// pixel isn't equal, but there was a run going, so we have to end it
			write_run(dest, e);
			// we still have to figure out how to encode this pixel
		}

		uint8_t hash = pixel_hash(p);
		if (pixel_equal(p, e->seen_pixels[hash])) {
			// pixel is in our table, so use the index
			write_index(dest, e, hash);
			// remember this pixel
			e->last_pixel = p;
			// handled
			continue;
		} else {
			// store pixel in table
			e->seen_pixels[hash] = p;
		}

		pixel_difference_t diff = pixel_subtract(p, e->last_pixel);
		// remember this pixel
		e->last_pixel = p;
		if (op_diff_compatible(&diff)) {
			// QOI_OP_DIFF
			write_diff(dest, e, &diff);
			// handled
			continue;
		} else if (op_luma_compatible(&diff)) {
			// QOI_OP_LUMA
			write_luma(dest, e, &diff);
			// handled
			continue;
		}

		// now we just have to encode the whole pixel
		if (diff.a != 0) {
			write_rgba(dest, e, &p);
		} else {
			write_rgb(dest, e, &p);
		}
	}
}

//
// Write the end marker to a QOI file.
//
// dest: file to write to
//
void encoder_finish(FILE *dest) {
	uint8_t marker[] = QOI_END_MARKER;
	fwrite(marker, sizeof(uint8_t), sizeof(marker), dest);
}

//
// Get statistics from the operation of a QOI encoder. If the encoder was not set to track
// statistics, this function returns NULL.
//
// e: QOI encoder to inspect
//
qoi_stats_t *encoder_get_stats(Encoder *e) {
	if (e->track_stats) {
		return &e->stats;
	} else {
		return NULL;
	}
}

//
// Free an Encoder and set the passed pointer to NULL.
//
// e: double pointer to the QOI encoder
//
void encoder_delete(Encoder **e) {
	if (e && *e) {
		free(*e);
		*e = NULL;
	}
}
