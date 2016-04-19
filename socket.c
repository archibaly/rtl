#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "socket.h"
#include "debug.h"

void socket_set_non_blocking(int sockfd)
{
	int flags;

	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
		ERROR("fcntl()");
		abort();
	}

	flags |= O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, flags) == -1) {
		ERROR("fcntl()");
		abort();
	}
}

static void socket_reuse_endpoint(int sockfd)
{
	int reuse = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0) {
		/*
		 * if we cannot set an option then there's something very wrong
		 * with the system, so we abort the application
		 */
		ERROR("setsockopt()");
		abort();
	}
}

int socket_create(int type)
{
	int sockfd;
	/*
	 * Don't let the system abort the application when it tries to send bytes
	 * through a connection already closed by the client
	 */
	signal(SIGPIPE, SIG_IGN);

	if ((sockfd = socket(AF_INET, type, 0)) < 0) {
		ERROR("socket()");
		abort();
	}
	socket_reuse_endpoint(sockfd);
	return sockfd;
}

void socket_bind(int sockfd, unsigned short port)
{
	struct sockaddr_in server_addr;

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		ERROR("bind()");
		abort();
	}
}

void socket_start_listening(int sockfd)
{
	if (listen(sockfd, SOMAXCONN) == -1) {
		ERROR("listen()");
		abort();
	}
}

/**
 * get_ip - get ip by hostname
 * @hostname: in
 * @ip: out
 */
static int get_ip(const char *hostname, char *ip)
{
	struct hostent *ht;
	ht = gethostbyname(hostname);

	if (ht == NULL)
		return -1;

	if (!inet_ntop(AF_INET, ht->h_addr_list[0], ip, INET_ADDRSTRLEN))
		return -1;

	return 0;
}

/**
 * socket_connect - connect to tcp server
 * @host: hostname or ip of the server
 */
int socket_connect(const char *host, uint16_t port)
{
	char ip[INET_ADDRSTRLEN];
	int sockfd;
	struct sockaddr_in server_addr;

	if (host_is_ipv4(host)) {
		strcpy(ip, host);
	} else {
		if (get_ip(host, ip) < 0) {
			return -1;
		}
	}

	sockfd = socket_create(TCP);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &server_addr.sin_addr) < 0) {
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		return -1;

	return sockfd;
}

void tcp_server_init(uint16_t port)
{
	int sockfd;

	sockfd = socket_create(TCP);
	socket_bind(sockfd, port);
	socket_start_listening(sockfd);
}

int socket_recv(int sockfd, void *buff, int size)
{
	int n;

	for (;;) {
		if ((n = recv(sockfd, buff, size, 0)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else {
			return n;
		}
	}
}

int socket_send(int sockfd, const void *buff, int size)
{
	int nleft;
	int nsent;
	const char *ptr;

	ptr = buff;
	nleft = size;

	while (nleft > 0) {
		if ((nsent = send(sockfd, ptr, nleft, 0)) <= 0) {
			if (nsent < 0 && errno == EINTR)
				nsent = 0;		/* and call write() again */
			else
				return -1;		/* error */
		}

		nleft -= nsent;
		ptr   += nsent;
	}
	return size;
}
