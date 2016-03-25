#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "socket.h"

void socket_set_non_blocking(int sockfd)
{
	int flags;

	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
	#ifdef DEBUG
		perror("fcntl");
	#endif
		abort();
	}

	flags |= O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, flags) == -1) {
	#ifdef DEBUG
		perror("fcntl");
	#endif
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
	#ifdef DEBUG
		perror("setsockopt");
	#endif
		abort();
	}
}


int socket_create(void)
{
	int sockfd;
	/*
	 * Don't let the system abort the application when it tries to send bytes
	 * through a connection already closed by the client
	 */
	signal(SIGPIPE, SIG_IGN);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	#ifdef DEBUG
		perror("socket");
	#endif
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
	#ifdef DEBUG
		perror("bind");
	#endif
		abort();
	}
}


void socket_start_listening(int sockfd)
{
	if (listen(sockfd, SOMAXCONN) == -1) {
	#ifdef DEBUG
		perror("listen");
	#endif
		abort();
	}
}
