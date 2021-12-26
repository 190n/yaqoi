#include "encoder.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(void) {
	Encoder *e = encoder_create(true, &(qoi_desc_t) {
	                                      .width = 2,
	                                      .height = 2,
	                                      .channels = RGB,
	                                      .colorspace = QOI_SRGB,
	                                  });
	encoder_write_header(stdout, e);
	encoder_delete(&e);
	fwrite("\xfe\xff\x00\x00\xfe\x00\xff\x00\xfe\x00\xff\x00", 1, 12, stdout);
	fwrite("\x00\x00\x00\x00\x00\x00\x00\x01", 1, 8, stdout);
	return 0;
}
