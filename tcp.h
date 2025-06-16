
struct tcp_sock {
	char* ipv4;
	char* port;
	int sock;
};

struct tcp_sock open_tcp(char* ipv4, char* port, int backlog);
void close_tcp(struct tcp_sock *conn);
