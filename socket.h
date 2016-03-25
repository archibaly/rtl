#ifndef _SOCKET_H_
#define _SOCKET_H_

int socket_create(void);
void socket_bind(int sockfd, unsigned short port);
void socket_set_non_blocking(int sockfd);
void socket_start_listening(int sockfd);

#endif	/* _SOCKET_H_ */
