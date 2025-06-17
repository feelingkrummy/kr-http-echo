#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include "tcp.h"

struct tcp_sock open_tcp(char* ipv4, char* port, int backlog) {
	struct addrinfo hints = {0};
	struct addrinfo *servinfo = NULL;

	struct tcp_sock conn = {
		.addr = 0,
		.port = 0,
		.sock = -1
	};

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int s = getaddrinfo(ipv4, port, &hints, &servinfo);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo error : (%d) %s\n", s, gai_strerror(s));
		return conn;
	}

	if ((conn.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		int e = errno;
		fprintf(stderr, "Cannot open socket : (%d) %s\n", e, strerror(e));
		goto error;
	}

	int enable = 1;
	if (setsockopt(conn.sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable) == -1) {
		int e = errno;
		fprintf(stderr, "Cannot set options : (%d) %s\n", errno, strerror(errno));
		goto error;
	}

	if (bind(conn.sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		int e = errno;
		fprintf(stderr, "Cannot bind socket : (%d) : %s\n", e, strerror(e));
		goto error;
	}

	// All this just to get a host ordered ipv4 address and a port :(
	struct sockaddr_in info_copy = {0};
	memcpy(&info_copy, servinfo->ai_addr, sizeof info_copy);
	conn.addr = ntohl(info_copy.sin_addr.s_addr);
	conn.port = ntohs(info_copy.sin_port);

	if (listen(conn.sock, backlog) == -1) {
		int e = errno;
		fprintf(stderr, "Cannot listened to socket : (%d) %s\n", e, strerror(e));
		goto error;
	}

	freeaddrinfo(servinfo);
	return conn;

error:
	// If socket was openned and we had an error,
	// close socket to avoid partial initialization
	close_tcp(&conn);
	freeaddrinfo(servinfo);
	return conn;
}

void close_tcp(struct tcp_sock *conn) {
	if (conn->sock != -1) {
		close(conn->sock);
		conn->sock = -1;
	}
	conn->addr = 0;
	conn->port = 0;
}
