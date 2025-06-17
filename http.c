#include <assert.h>

#include "http.h"
#include "string.h"

const char not_found_msg_data[] = 
	"HTTP/1.1 404 Not Found\r\n"
	"Connection : close \r\n"
	"\r\n"
;

const struct slice8 not_found_msg = {
	.len = len_of_char_array(not_found_msg_len),
	.ptr = not_found_msg_data
};

static int check_if_complete_request(struct string8 request) {

}

struct string8 read_http_request(int fd) {
	assert(fd != -1);

	struct string8 req = {
		.len = 0,
		.cap = 4096,
		.ptr = calloc(4096, sizeof(char));
	}

	int complete = 0;
	int rd_bytes = 0;
	while (!complete) {
		rd_bytes = recv(fd, req.ptr, req.cap, 0);
		req.len += rd_bytes;
		complete = check_if_complete_request(req);
		
	};
}
