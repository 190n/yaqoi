#include "encoder.h"

int main(void) {
	Encoder *e = encoder_create(false, &(qoi_desc_t) {
		.width = 0x01234567,
		.height = 0xabcdef01,
		.channels = RGB,
		.colorspace = QOI_LINEAR,
	});
	encoder_write_header(stdout, e);
	encoder_delete(&e);
	return 0;
}
