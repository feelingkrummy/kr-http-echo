#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define KR_LOCALHOST "127.0.0.1"
#define KR_PORT "18080"
#define KR_BACKLOG 10

static volatile sig_atomic_t running = 1;

static void sigint_handler(int n) {
	running = 0;
}

int open_tcp_conn(const char* ipv4, const char* port) {
	struct addrinfo hints = {0};
	struct addrinfo *servinfo = NULL;
	int result = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(ipv4, port, &hints, &servinfo);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo error : (%d) %s\n", status, gai_strerror(status));
		return -1;
	}
	fprintf(stderr, "Got addrinfo\n");

	// Start Connection
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "Cannot open socket : (%d) %s\n", errno, strerror(errno));
		freeaddrinfo(servinfo);
		return -1;
	}
	fprintf(stderr, "Openned socket!\n");

	// I set reuseaddr so that I can interate more quickly. Even if the code
	// closes the socket correctly after getting SIGINT, I can't immediately
	// start the server up again because "The addr is still in use"
	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable) == -1) {
		int e = errno;
		fprintf(stderr, "Cannot set options : (%d) %s\n", errno, strerror(errno));
		freeaddrinfo(servinfo);
		return -1;
	}

	result = bind(sock, servinfo->ai_addr, servinfo->ai_addrlen);
	if (result == -1) {
		fprintf(stderr, "Cannot bind socket : (%d) : %s\n", errno, strerror(errno));
		freeaddrinfo(servinfo);
		close(sock);
		return -1;
	}

	result = listen(sock, KR_BACKLOG);
	if (result == -1) {
		fprintf(stderr, "Cannot listend to socket : (%d) : %s\n", errno, strerror(errno));
		freeaddrinfo(servinfo);
		close(sock);
		return -1;
	}

	return sock;
}

void close_tcp_conn(int sockfd) {
	if (close(sockfd) == -1) {
		int e = errno;
		fprintf(stderr, "Could not close socket, (%d) %s\n", e, strerror(e));
	}
}

#define RD_BUF_LEN 4096

static char rd_buf[RD_BUF_LEN] = {0};

int is_get_root(char* buf, int buf_len) {
	char req[] = "GET / HTTP/1.1";
	int req_len = (sizeof req)-1;

	int min_len = (req_len < buf_len) ? req_len : buf_len;
	fprintf(stderr, "%d ? %d : %d\n", min_len, req_len, buf_len);

	if (memcmp(req, buf, min_len) == 0) {
		return 1;
	} else {
		fprintf(stderr, "(%d), \"%.*s\" != \"%.*s\"\n", min_len, min_len, req, min_len, buf);
		return 0;
	}
}

int main(int argc, char* argv[]) {
	struct sigaction sigint_sa = {0};
	
	memset(&sigint_sa, 0, sizeof(sigint_sa));
	sigint_sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sigint_sa, NULL);

	int sockfd = open_tcp_conn(KR_LOCALHOST, KR_PORT);
	if (sockfd == -1) {
		fprintf(stderr, "Could not open socket, exitting ... \n");
		exit(1);
	}

	// This is wrong
	while (running) {
		struct sockaddr_storage their_addr;
		socklen_t addr_size = sizeof their_addr;
		int accept_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

		if (accept_fd == -1) {
			int e = errno;
			if (e != EINTR) {
				fprintf(stderr, "Cannot accept : (%d) : %s\n", errno, strerror(errno));
				close_tcp_conn(sockfd);
				exit(1);
			}
			continue;
		}

		char resp_msg[] =
			"HTTP/1.1 200 OK\r\n"
			"Connection : close\r\n"
			"Content-Type : text/html; charset=UTF-8\r\n"
			"Content-Length : 88\r\n"
			"\r\n"
			"<html><head><title>Hello, world!</title></head><body><h1>Hello, world!</h1><body></html>"
		;
		int resp_msg_len = sizeof resp_msg;

		char not_found_msg[] =
			"HTTP/1.1 404 Not Found\r\n"
			"Connection : close \r\n"
			"\r\n"
		;
		int not_found_msg_len = sizeof not_found_msg;

		int rd_bytes = recv(accept_fd, rd_buf, RD_BUF_LEN-1, 0);

		if (rd_bytes > 0) {
			fprintf(stderr, "(%d) [\n%s\n]\n", rd_bytes, rd_buf);
		} else if ( rd_bytes == 0 ) {
			fprintf(stderr, "Remote side closed connection!\n");
			continue;
		} else {
			fprintf(stderr, "Cannot recv : (%d) : %s\n", errno, strerror(errno));
			running = 0;
			close(accept_fd);
			continue;
		}

		int wr_bytes = 0;
		if (is_get_root(rd_buf, rd_bytes)) {
			fprintf(stderr, "Responding to GET ... \n");
			fprintf(stderr, "resp_msg_len : %d\n", resp_msg_len);
			wr_bytes = send(accept_fd, resp_msg, resp_msg_len, 0);
		} else {
			fprintf(stderr, "Request not found ... \n");
			fprintf(stderr, "not_found_msg_len : %d\n", not_found_msg_len);
			wr_bytes = send(accept_fd, not_found_msg, not_found_msg_len, 0);
		}

		fprintf(stderr, "wr_bytes : %d\n", wr_bytes);
		if (wr_bytes == -1) {
			fprintf(stderr, "Cannot send : (%d) : %s\n", errno, strerror(errno));
			running = 0;
			close(accept_fd);
			continue;
		}

		close(accept_fd);
	}

	fprintf(stderr, "Exiting ... \n");

	close_tcp_conn(sockfd);

	return 0;
}
