#include "encoder.h"

int main(void) {
	Encoder *e = encoder_create(false, &(qoi_desc_t) {
		.width = 1024,
		.height = 2048,
		.channels = RGB,
		.colorspace = QOI_SRGB,
	});
	encoder_write_header(stdout, e);
	encoder_delete(&e);
	return 0;
}
