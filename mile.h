#ifndef __MILE_H__
#define __MILE_H__

#include <stdint.h>

typedef struct Mile Mile;

Mile *mile_open_file(const char *filename);

Mile *mile_open_memory(uint8_t *buf, uint64_t buf_size);

int mile_write(Mile *dest, uint8_t *buf, uint64_t nbytes);

#endif
