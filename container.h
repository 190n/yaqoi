#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <stdio.h>

#include "qoi.h"

void write_header(FILE *dest, qoi_desc_t *desc);

void write_end_marker(FILE *dest);

#endif
