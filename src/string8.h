#ifndef _STRING_H_
#define _STRING_H_

#include <stdint.h>

#define length_of(s) sizeof(s)-1

struct string8 {
	uint64_t len;
	uint64_t cap;
	char* ptr;
};

void destroy_string8(struct string8 *s);

#endif
