#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <stdint.h>
#include "url.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN		16	/* xxx.xxx.xxx.xxx\0 */
#endif

#define TCP	SOCK_STREAM
#define UDP	SOCK_DGRAM

int socket_create(int type);
void socket_bind(int sockfd, unsigned short port);
void socket_set_non_blocking(int sockfd);
void socket_start_listening(int sockfd);
void tcp_server_init(uint16_t port);
int socket_connect(const char *host, uint16_t port);
int socket_recv(int sockfd, void *buff, int size);
int socket_send(int sockfd, const void *buff, int size);

#endif	/* _SOCKET_H_ */
