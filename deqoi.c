#include "bits.h"
#include "pixel.h"

#include <inttypes.h>
#include <stdio.h>

int main(void) {
	pixel_t a, b;
	store_u32be((uint8_t *) &a.rgba, 0xff0000ff);
	store_u32be((uint8_t *) &b.rgba, 0xfe0100ff);
	pixel_difference_t diff = pixel_subtract(b, a);
	printf("%" PRIu8 " %" PRIu8 " %" PRIu8 " %" PRIu8 "\n", b.channels.r, b.channels.g,
	    b.channels.b, b.channels.a);
	printf("%" PRId16 " %" PRId16 "\n", diff.r, diff.g);
	return 0;
}
