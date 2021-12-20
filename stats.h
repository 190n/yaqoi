#ifndef __STATS_H__
#define __STATS_H__

typedef struct {
	uint64_t total_pixels;
	uint64_t total_bits;
	struct {
		uint64_t rgb, rgba, index, diff, luma, run;
	} op_to_pixels;
} qoi_stats_t;

#endif
