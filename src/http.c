#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#include "http.h"
#include "string8.h"

const char not_found_msg_data[] = 
	"HTTP/1.1 404 Not Found\r\n"
	"Connection : close \r\n"
	"\r\n"
;

const struct string8 not_found_msg = {
	.len = length_of(not_found_msg_data),
	.cap = sizeof not_found_msg_data,
	.ptr = not_found_msg_data
};

int check_if_complete_request(struct string8 request) {
	char* empty_line = NULL;
	if ((empty_line = strstr(request.ptr, "\r\n\r\n")) == NULL) {
		fprintf(stderr, "Empty line not found!\n");
		return 0;
	}
	fprintf(stderr, "Empty line found!\n");

	uint64_t content_length = 0;
	char* content_length_field = NULL;
	if ((content_length_field = strstr(request.ptr, "Content-Length")) != NULL) {
		fprintf(stderr, "Found content length\n");
		uint32_t pos = content_length_field - request.ptr;
		for (; pos < request.len; pos++) {
			if (request.ptr[pos] == ':') {
				pos++;
				break;
			}
		}
		content_length = strtol(&(request.ptr[pos]), NULL, 0);
		fprintf(stderr, "\"%.*s\"(%d), len = %lu\n", request.len-pos, &(request.ptr[pos]), request.len-pos, content_length);

		// Subtract position in string from string length to get length of body
		uint64_t body_len = request.len - (empty_line + 4 - request.ptr);

		if (body_len < content_length) {
			return 0;
		}
	}

	return 1;
}

struct string8 read_http_request(int fd) {
	assert(fd != -1);

	struct string8 req = {
		.len = 0,
		.cap = 4096,
		.ptr = calloc(4096, 1)
	};

	int complete = 0;
	int rd_bytes = 0;
	while (!complete) {
		rd_bytes = recv(fd, req.ptr, req.cap-1, 0);
		req.len += rd_bytes;
		complete = check_if_complete_request(req);
	};
}
