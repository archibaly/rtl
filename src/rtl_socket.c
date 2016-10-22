#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "rtl_debug.h"
#include "rtl_socket.h"

int rtl_socket_set_non_blocking(int sockfd)
{
	int flags;

	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
		rtl_debug("fcntl error: %s", strerror(errno));
		return -1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, flags) == -1) {
		rtl_debug("fcntl error: %s", strerror(errno));
		return -1;
	}

	return 0;
}

static int socket_reuse_endpoint(int sockfd)
{
	int reuse = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0) {
		rtl_debug("setsockopt error: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int rtl_socket_create(int type)
{
	int sockfd;
	/*
	 * Don't let the system abort the application when it tries to send bytes
	 * through a connection already closed by the client
	 */
	signal(SIGPIPE, SIG_IGN);

	if ((sockfd = socket(AF_INET, type, 0)) < 0) {
		rtl_debug("socket error: %s", strerror(errno));
		return -1;
	}

	if (socket_reuse_endpoint(sockfd) < 0 )
		return -1;

	return sockfd;
}

int rtl_socket_bind(int sockfd, int port)
{
	struct sockaddr_in server_addr;

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		rtl_debug("bind error: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int rtl_socket_start_listening(int sockfd)
{
	if (listen(sockfd, SOMAXCONN) == -1) {
		rtl_debug("listen error: %s", strerror(errno));
		return -1;
	}
	return 0;
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

static int host_is_ipv4(const char *str)
{
	if (str == NULL)
		return 0;
	while (*str) {
		if (isdigit(*str) || *str == '.')
			str++;
		else
			return 0;
	}
	return 1;
}

/**
 * socket_connect - connect to tcp server
 * @host: hostname or ip of the server
 */
int rtl_socket_connect(const char *host, int port)
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

	if ((sockfd = rtl_socket_create(RTL_TCP)) < 0)
		return -1;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &server_addr.sin_addr) < 0) {
		rtl_debug("inet_pton error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		rtl_debug("connect error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int rtl_tcp_server_init(int port)
{
	int sockfd;

	sockfd = rtl_socket_create(RTL_TCP);
	if (sockfd < 0)
		return -1;
	if (rtl_socket_bind(sockfd, port) < 0)
		return -1;
	if (rtl_socket_start_listening(sockfd) < 0)
		return -1;

	return sockfd;
}

int rtl_socket_accept(int sockfd, char *client_addr, size_t size)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int fd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
	if (fd < 0)
		return -1;

	if (!inet_ntop(AF_INET, &addr.sin_addr, client_addr, size)) {
		close(fd);
		return -1;
	}

	return fd;
}

int rtl_socket_recvn(int sockfd, void *buff, int size)
{
	int nrevd;
	int nleft = size;
	char *ptr = buff;

	while (nleft > 0) {
		if ((nrevd = recv(sockfd, ptr, nleft, 0)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nrevd == 0) {
			break;			/* EOF */
		}
		nleft -= nrevd;
		ptr   += nrevd;
	}

	return size - nleft;
}

int rtl_socket_sendn(int sockfd, const void *buff, int size)
{
	int nsent;
	int nleft = size;
	const char *ptr = buff;

	while (nleft > 0) {
		if ((nsent = send(sockfd, ptr, nleft, 0)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nsent == 0) {
			break;
		}

		nleft -= nsent;
		ptr   += nsent;
	}

	return size - nleft;
}
