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
	bool header_written;
	uint64_t pixels_written;
	bool end_marker_written;
};

//
// Create a new QOI encoder.
//
// track_stats: whether to track statistics during encoding
// desc:        information about the file to encode
//
Encoder *encoder_create(qoi_desc_t *desc) {
	// use calloc for zero initialization
	Encoder *e = (Encoder *) calloc(1, sizeof(Encoder));
	if (e) {
		e->desc = *desc;
		e->stats.total_pixels = ((uint64_t) desc->width) * ((uint64_t) desc->height);
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
		if (e->header_written) {
			return;
		}
		e->header_written = true;
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
	e->stats.total_bits += 8;
	e->stats.op_to_pixels.run += e->run_length;
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
	e->stats.total_bits += 8;
	e->stats.op_to_pixels.index++;
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
	if (e->end_marker_written) {
		return;
	}

	for (uint64_t i = 0; i < n; i++) {
		pixel_t p = pixels[i];
		if (pixel_equal(p, e->last_pixel)) {
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
			// handled
			continue;
		} else {
			// store pixel in table
			e->seen_pixels[hash] = p;
		}
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
