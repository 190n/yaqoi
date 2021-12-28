#include "encoder.h"
#include "stb_image.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

#define OPTIONS "hvi:o:"

FILE *infile, *outfile;
unsigned char *data = NULL;
Encoder *e = NULL;

void cleanup() {
	fclose(infile);
	fclose(outfile);
	if (data) {
		stbi_image_free(data);
	}
	if (e) {
		encoder_delete(&e);
	}
}

void usage(const char *program_name) {
	fprintf(stderr,
	    "usage: %s [-hv] [-i input] [-o output]\n"
	    "    -h:        show usage\n"
	    "    -v:        print encoding statistics\n"
	    "    -i input:  specify input file. must be PNG, PPM, or PGM. default is stdin.\n"
	    "    -o output: specify output file. default is stdout.\n",
	    program_name);
}

int main(int argc, char **argv) {
	infile = stdin;
	outfile = stdout;
	bool verbose = false;
	int opt = 0;

	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'h':
				usage(argv[0]);
				return 1;
			case 'v':
				verbose = true;
				break;
			case 'i':
				infile = fopen(optarg, "rb");
				if (!infile) {
					fprintf(stderr, "%s: %s: ", argv[0], optarg);
					perror("");
					cleanup();
					return 1;
				}
				break;
			case 'o':
				outfile = fopen(optarg, "wb");
				if (!outfile) {
					fprintf(stderr, "%s: %s: ", argv[0], optarg);
					perror("");
					cleanup();
					return 1;
				}
				break;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	(void) verbose;

	int x, y, n;
	data = stbi_load_from_file(infile, &x, &y, &n, 4);
	if (!data) {
		fprintf(stderr, "%s: failed to decode input: %s\n", argv[0], stbi_failure_reason());
		cleanup();
		return 1;
	}

	// 2 channels = gray + alpha
	// 4 channels = RGBA
	qoi_channels_t channels = (n == 2 || n == 4) ? RGBA : RGB;

	e = encoder_create(verbose, &(qoi_desc_t) {
	                                .width = x,
	                                .height = y,
	                                .colorspace = QOI_SRGB,
	                                .channels = channels,
	                            });
	if (!e) {
		fprintf(stderr, "%s: failed to create QOI encoder\n", argv[0]);
		cleanup();
		return 1;
	}

	encoder_write_header(outfile, e);
	encoder_encode_pixels(outfile, e, (pixel_t *) data, ((uint64_t) x) * ((uint64_t) y));
	encoder_finish(outfile, e);

	cleanup();
	return 0;
}
