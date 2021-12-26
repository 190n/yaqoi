#include "encoder.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_PNM
// #define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

#define OPTIONS "hvi:o:"

int main(int argc, char **argv) {
	FILE *infile = stdin, *outfile = stdout;
	bool verbose = false;
	int opt = 0;

	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch (opt) {
			case 'h':
				fprintf(stderr, "help\n");
				return 1;
			case 'v':
				verbose = true;
				break;
			case 'i':
				infile = fopen(optarg, "rb");
				if (infile == NULL) {
					fprintf(stderr, "%s: %s: ", argv[0], optarg);
					perror("");
					return 1;
				}
				break;
			case 'o':
				outfile = fopen(optarg, "wb");
				if (outfile == NULL) {
					fprintf(stderr, "%s: %s: ", argv[0], optarg);
					perror("");
					return 1;
				}
				break;
			default:
				fprintf(stderr, "help\n");
				return 1;
		}
	}

	(void) verbose;

	int x, y, n;
	unsigned char *data = stbi_load_from_file(infile, &x, &y, &n, 0);
	if (data == NULL) {
		fprintf(stderr, "%s: failed to decode input: %s\n", argv[0], stbi_failure_reason());
	}

	fprintf(stderr, "w, h, c = %d, %d, %d\n", x, y, n);

	stbi_image_free(data);
	fclose(infile);
	fclose(outfile);
	return 0;
}
