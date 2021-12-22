#include "encoder.h"

#include <stdlib.h>
#include <string.h>

#include "pixel.h"

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
		memcpy(h.magic, "qoif", 4);
		h.width = e->desc.width;
		h.height = e->desc.height;
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
