#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

#include "string8.h"
#include "tcp.h"
#include "http.h"

#define KR_LOCALHOST "127.0.0.1"
#define KR_PORT "18080"
#define KR_BACKLOG 10

static volatile sig_atomic_t running = 1;

static void sigint_handler(int n) {
	running = 0;
}

struct string8 generate_response(struct string8 request) {
	struct string8 resp = {
		.len = 0,
		.cap = 4096,
		.ptr = calloc(4096, 1)
	};
	struct string8 body = {
		.len = 0,
		.cap = request.len + 1024,
		.ptr = calloc(request.len + 1024, 1)
	};
	int body_bytes = 0;
	char body_template[] = 
		"<html>\n"
		"\t<head><title>KR Echo Server</title></head>\n"
		"\t<body>\n"
		"\t\t<h1>Request:</h1>\n"
		"\t\t<pre>\n%.*s"
		"\t\t</pre>\n"
		"\t</body>\n"
		"</html>"
	;

	body_bytes = snprintf(body.ptr, body.cap, body_template, request.len, request.ptr);
	body.len = body_bytes;
	// If we can't fit the formatted string into the body string, something
	// strange has happend and I'd rather just crash than deal with that
	// corner case
	assert(body_bytes < body.cap);

	char header[] = 
		"HTTP/1.1 200 OK\r\n"
		"Connection : close\r\n"
		"Content-Type : text/html; charset=UTF-8\r\n"
		"Content-Length : %lu\r\n"
		"\r\n"
	;

	int b = snprintf(resp.ptr, resp.cap-1, header, body.len);
	resp.len += b;

	fprintf(stderr, "header : [\n%.*s]\n", resp.len, resp.ptr);
	fprintf(stderr, "body : (%d) [\n%.*s]\n", body.len, body.len, body.ptr);

	while (resp.len + body.len >= resp.cap) {
		fprintf(stderr, "resizing array\n");
		size_t new_cap = resp.cap*1.5;
		char* new = calloc(new_cap, 1);
		assert(new);
		memcpy(new, resp.ptr, resp.cap);
		free(resp.ptr);
		resp.ptr = new;
		resp.cap = new_cap;
	}

	char* wr_ptr = resp.ptr + resp.len;
	fprintf(stderr, "{%lu, %lu, %p}, wr_ptr = %p\n", resp.len, resp.cap, resp.ptr, wr_ptr);
	fprintf(stderr, "{%lu, %lu, %p}\n", body.len, body.cap, body.ptr);
	memcpy(wr_ptr, body.ptr, body.len);
	resp.len = resp.len + body.len;

	fprintf(stderr, "resp : [\n%.*s]\n", resp.len, resp.ptr);

	destroy_string8(&body);
	return resp;
}

int main(int argc, char* argv[]) {
	struct sigaction sigint_sa = {0};
	
	memset(&sigint_sa, 0, sizeof(sigint_sa));
	sigint_sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sigint_sa, NULL);

	struct tcp_sock conn = open_tcp(KR_LOCALHOST, KR_PORT, KR_BACKLOG);
	if (conn.sock == -1) {
		fprintf(stderr, "Could not open socket, exitting ... \n");
		exit(1);
	}

	while (running) {
		struct sockaddr_storage their_addr;
		socklen_t addr_size = sizeof their_addr;
		int accept_fd = accept(conn.sock, (struct sockaddr *)&their_addr, &addr_size);

		if (accept_fd == -1) {
			int e = errno;
			if (e != EINTR || running == 1) {
				fprintf(stderr, "Cannot accept : (%d) : %s\n", errno, strerror(errno));
				close_tcp(&conn);
				exit(1);
			}
			continue;
		}

		struct string8 request = read_http_request(accept_fd);
		if (request.ptr == NULL) {
			running = 0;
			destroy_string8(&request);
			close(accept_fd);
			continue;
		}

		fprintf(stderr, "Received request : [\n%.*s]\n", request.len, request.ptr);

		struct string8 response = {
			.len = 0,
			.cap = 0,
			.ptr = NULL
		};
		char root_header[] = "GET / HTTP/1.1";

		int result = 0;
		if ((result = memcmp(root_header, request.ptr, (sizeof root_header)-1)) == 0) {
			fprintf(stderr, "Get / request\n");
			response = generate_response(request);
		} else {
			fprintf(stderr, "Not GET / request, %d\n", result);
			response.ptr = calloc(not_found_msg.cap, 1);
			response.cap = not_found_msg.cap;
			response.len = not_found_msg.len;
			memcpy(response.ptr, not_found_msg.ptr, response.cap);
		}

		int written = write_http_response(accept_fd, response);
		if (written != not_found_msg.len) {
			fprintf(stderr, "Cound not complete http response\n");
			running = 0;
		}

		destroy_string8(&response);
		destroy_string8(&request);
		close(accept_fd);
	}

	fprintf(stderr, "Exiting ... \n");

	close_tcp(&conn);

	return 0;
}
