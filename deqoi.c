#include "bits.h"
#include "pixel.h"

#include <inttypes.h>
#include <stdio.h>

int main(void) {
	pixel_t a;
	a.channels.r = a.channels.g = a.channels.b = a.channels.a = 255;
	pixel_t b = a;
	b.channels.r = b.channels.g = b.channels.b = 0x0f;
	pixel_difference_t diff = pixel_subtract(b, a);
	printf("%" PRIu8 " %" PRIu8 " %" PRIu8 " %" PRIu8 "\n", b.channels.r, b.channels.g,
	    b.channels.b, b.channels.a);
	printf("%" PRIu8 " %" PRIu8 " %" PRIu8 " %" PRIu8 "\n", diff.r, diff.g, diff.b, diff.a);
	return 0;
}
