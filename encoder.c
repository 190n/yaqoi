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
