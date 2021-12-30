#include "container.h"
#include "encoder.h"
#include "stb_image.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define OPTIONS "hlvi:o:"

FILE *infile = NULL, *outfile = NULL;
unsigned char *data = NULL;
Encoder *e = NULL;

void cleanup() {
	if (infile) {
		fclose(infile);
	}
	if (outfile) {
		fclose(outfile);
	}
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
		"    -l:        indicate that output file is linear sRGB as opposed to gamma. note that no colorspace conversion is performed.\n"
	    "    -v:        print encoding statistics\n"
	    "    -i input:  specify input file. default is stdin.\n"
	    "    -o output: specify output file. default is stdout.\n",
	    program_name);
}

int main(int argc, char **argv) {
	infile = stdin;
	outfile = stdout;
	bool verbose = false, linear_srgb = false;
	int opt = 0;

	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'h':
				usage(argv[0]);
				return 1;
			case 'l':
				linear_srgb = true;
				break;
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
	qoi_desc_t desc = {
		.width = x,
		.height = y,
		.colorspace = linear_srgb ? QOI_LINEAR : QOI_SRGB,
		.channels = channels,
	};

	e = encoder_create(verbose, &desc);
	if (!e) {
		fprintf(stderr, "%s: failed to create QOI encoder\n", argv[0]);
		cleanup();
		return 1;
	}

	write_header(outfile, &desc);
	clock_t start = clock();
	encoder_encode_pixels(outfile, e, (pixel_t *) data, ((uint64_t) x) * ((uint64_t) y));
	clock_t end = clock();
	encoder_finish(outfile, e);
	write_end_marker(outfile);

	if (verbose) {
		qoi_stats_t *stats = encoder_get_stats(e);
		double bpp = (double) stats->total_bits / stats->total_pixels,
		       percent_rgb = 100.0 * stats->op_to_pixels.rgb / stats->total_pixels,
		       percent_rgba = 100.0 * stats->op_to_pixels.rgba / stats->total_pixels,
		       percent_index = 100.0 * stats->op_to_pixels.index / stats->total_pixels,
		       percent_diff = 100.0 * stats->op_to_pixels.diff / stats->total_pixels,
		       percent_luma = 100.0 * stats->op_to_pixels.luma / stats->total_pixels,
		       percent_run = 100.0 * stats->op_to_pixels.run / stats->total_pixels;

		double encode_time = ((double) (end - start)) / CLOCKS_PER_SEC,
		       speed = stats->total_pixels / encode_time / 1000000.0;

		fprintf(stderr,
		    "BPP:   %f\n"
		    "speed: %f MP/s\n"
		    "operator usage by number of pixels:\n"
		    "    QOI_OP_RGB:   %f%%\n"
		    "    QOI_OP_RGBA:  %f%%\n"
		    "    QOI_OP_INDEX: %f%%\n"
		    "    QOI_OP_DIFF:  %f%%\n"
		    "    QOI_OP_LUMA:  %f%%\n"
		    "    QOI_OP_RUN:   %f%%\n",
		    bpp, speed, percent_rgb, percent_rgba, percent_index, percent_diff, percent_luma,
		    percent_run);
	}

	cleanup();
	return 0;
}
