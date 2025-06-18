#include <stdlib.h>

#include "string8.h"

void destroy_string8(struct string8 *s) {
	if (s->ptr != NULL) {
		free(s->ptr);
	}
	s->cap = 0;
	s->len = 0;
}
