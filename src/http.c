#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>

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
	.ptr = (char*)not_found_msg_data
};

int check_if_complete_request(struct string8 request) {
	char* empty_line = NULL;
	if ((empty_line = strstr(request.ptr, "\r\n\r\n")) == NULL) {
		// fprintf(stderr, "Empty line not found!\n");
		return 0;
	}
	// fprintf(stderr, "Empty line found!\n");

	uint64_t content_length = 0;
	char* content_length_field = NULL;
	if ((content_length_field = strstr(request.ptr, "Content-Length")) != NULL) {
		// fprintf(stderr, "Found content length\n");
		uint32_t pos = content_length_field - request.ptr;
		for (; pos < request.len; pos++) {
			if (request.ptr[pos] == ':') {
				pos++;
				break;
			}
		}
		content_length = strtol(&(request.ptr[pos]), NULL, 0);
		// fprintf(stderr, "\"%.*s\"(%d), len = %lu\n", request.len-pos, &(request.ptr[pos]), request.len-pos, content_length);

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
	assert(req.ptr);

	int complete = 0;
	int rd_bytes = 0;
	while (true) {
		char* dest = req.ptr+req.len; // Calculate current copy location
		size_t size = req.cap-req.len-1; // Calculate space in string
		rd_bytes = recv(fd, req.ptr + req.len, req.cap-req.len-1, 0);
		// Handle recv error cases, return empty string
		// If connection closes dump whatever we're working on
		if (rd_bytes <= 0) {
			if (rd_bytes < 0) {
				int e = errno;
				fprintf(stderr, "Cannot recv : (%d) %s\n", errno, strerror(errno));
			} else {
				fprintf(stderr, "Remote side closed connection!\n");
			}
			destroy_string8(&req);
			return req;
		}

		// Good read, process
		req.len += rd_bytes;
		complete = check_if_complete_request(req);
		if (!complete) {
			if (req.len+1 >= req.cap) {
				size_t new_cap = req.cap*1.5;
				char* new = calloc(new_cap, 1);
				assert(new);
				memcpy(new, req.ptr, req.cap);
				free(req.ptr);
				req.ptr = new;
				req.cap = new_cap;
			}
		} else {
			break;
		}
	};

	return req;
}

int write_http_response(int fd, struct string8 response) {
	int written = 0;
	while (written < response.len) {
		char* wr_ptr = &(response.ptr[written]);
		uint64_t wr_len = response.len - written;
		int wr_bytes = send(fd, wr_ptr, wr_len, 0);
		if (wr_bytes == -1) {
			int e = errno;
			fprintf(stderr, "Cannont send : (%d) %s\n", e, strerror(e));
			return written;
		}
		written += wr_bytes;
	}
}
