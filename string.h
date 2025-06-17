#ifndef _STRING_H_
#define _STRING_H_

#define len_of_char_array(s) sizeof(s)-1

struct string8 {
	uint64_t len;
	uint64_t cap;
	char* ptr;
};

struct slice8 {
	uint64_t len;
	char* ptr;
}

#endif
