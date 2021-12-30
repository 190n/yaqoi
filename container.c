#include "container.h"

#include "bits.h"

//
// Write the header of a QOI file.
//
// dest: file to write the header to
// desc: description of the QOI image
//
void write_header(FILE *dest, qoi_desc_t *desc) {
	qoi_header_t h;
	store_u32be(h.magic, QOI_MAGIC);
	store_u32be((uint8_t *) &h.width, desc->width);
	store_u32be((uint8_t *) &h.height, desc->height);
	h.channels = desc->channels;
	h.colorspace = desc->colorspace;
	fwrite(&h, QOI_HEADER_LENGTH, 1, dest);
}

//
// Write the end marker to a QOI file.
//
// dest: file to write to
//
void write_end_marker(FILE *dest) {
	uint8_t marker[] = QOI_END_MARKER;
	fwrite(marker, sizeof(uint8_t), QOI_END_MARKER_LENGTH, dest);
}
