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
		.ipv4 = ipv4,
		.port = port,
		.sock = -1,
	};

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int s = getaddrinfo(ipv4, port, &hints, &servinfo);
	if ( s != 0) {
		fprintf(stderr, "getaddrinfo error : (%d) %s\n", s, gai_strerror(s));
		return conn;
	}

	if ( (conn.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
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
	if (conn.sock != -1) {
		close(conn.sock);
		conn.sock = -1;
	}
	freeaddrinfo(servinfo);
	return conn;
}

void close_tcp(struct tcp_sock *conn) {
	if (conn->sock != -1) {
		close(conn->sock);
		conn.sock = -1;
	}
	// TODO : Do I not take ownership of strings (so just set to NULL) OR
	// does tcp_conn take ownsership of strings and I need to free(*)
	conn->ipv4 = NULL;
	conn->port = NULL:
}
