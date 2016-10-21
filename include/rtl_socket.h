#ifndef _RTL_SOCKET_H_
#define _RTL_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>

#ifndef RTL_INET_ADDRSTRLEN
#define RTL_INET_ADDRSTRLEN		16 /* xxx.xxx.xxx.xxx\0 */
#endif

#define RTL_TCP	SOCK_STREAM
#define RTL_UDP	SOCK_DGRAM

int rtl_socket_create(int type);
int rtl_socket_bind(int sockfd, int port);
int rtl_socket_set_non_blocking(int sockfd);
int rtl_socket_start_listening(int sockfd);
int rtl_tcp_server_init(int port);
int rtl_socket_accept(int sockfd, char *client_addr, size_t size);
int rtl_socket_connect(const char *host, int port);
int rtl_socket_recv(int sockfd, void *buff, int size);
int rtl_socket_send(int sockfd, const void *buff, int size);

#endif /* _RTL_SOCKET_H_ */
