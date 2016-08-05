#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN		16	/* xxx.xxx.xxx.xxx\0 */
#endif

#define TCP	SOCK_STREAM
#define UDP	SOCK_DGRAM

int socket_create(int type);
int socket_bind(int sockfd, int port);
int socket_set_non_blocking(int sockfd);
int socket_start_listening(int sockfd);
int tcp_server_init(int port);
int socket_connect(const char *host, int port);
int socket_recv(int sockfd, void *buff, int size);
int socket_send(int sockfd, const void *buff, int size);

#endif /* _SOCKET_H_ */
