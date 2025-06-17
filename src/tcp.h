#ifndef _TCP_SOCK_H_
#define _TCP_SOCK_H_

#include <sys/socket.h>
#include <netinet/in.h>

struct tcp_sock {
	union {
		in_addr_t addr;
		char addr_bytes[];
	};
	in_port_t port;
	int sock;
};

struct tcp_sock open_tcp(char* ipv4, char* port, int backlog);
void close_tcp(struct tcp_sock *conn);

#endif 
