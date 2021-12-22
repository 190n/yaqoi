#include "encoder.h"

#include "bits.h"
#include "pixel.h"

#include <stdlib.h>
#include <string.h>

struct Encoder {
	FILE *output;
	bool track_stats;
	pixel_t seen_pixels[64];
	qoi_stats_t stats;
	qoi_desc_t desc;
};

//
// Create a new QOI encoder.
//
// output:      output filename
// track_stats: whether to track statistics during encoding
// desc:        information about the file to encode
//
Encoder *encoder_create(const char *output, bool track_stats, qoi_desc_t *desc) {
	// use calloc for zero initialization
	Encoder *e = (Encoder *) calloc(1, sizeof(Encoder));
	if (e) {
		e->output = fopen(output, "wb");
		e->track_stats = track_stats;
		e->desc = *desc;
	}

	return e;
}

//
// Write the header of a QOI file.
//
// e: QOI encoder to use
//
void encoder_write_header(Encoder *e) {
	if (e) {
		qoi_header_t h;
		store_u32be(h.magic, QOI_MAGIC);
		store_u32be((char *) &h.width, e->desc.width);
		store_u32be((char *) &h.height, e->desc.height);
		h.channels = e->desc.channels;
		h.colorspace = e->desc.colorspace;
		fwrite(&h, sizeof(qoi_header_t), 1, e->output);
	}
}

//
// Free an Encoder and set the passed pointer to NULL.
//
// e: double pointer to the QOI encoder
//
void encoder_delete(Encoder **e) {
	if (e && *e) {
		fclose((*e)->output);
		free(*e);
		*e = NULL;
	}
}
