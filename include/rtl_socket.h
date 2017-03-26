#ifndef _RTL_SOCKET_H_
#define _RTL_SOCKET_H_

#include <stdint.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

enum rtl_socket_connect_type {
	SKT_TCP = 0,
	SKT_UDP,
	SKT_UNIX,
};

typedef struct rtl_socket_addr {
	uint32_t ip;
	uint16_t port;
} rtl_socket_addr_t;

typedef struct rtl_socket_naddr {
	uint32_t ip;
	uint16_t port;
} rtl_socket_naddr_t;

typedef struct rtl_socket_paddr {
	char ip[INET_ADDRSTRLEN];
	uint16_t port;
} rtl_socket_saddr_t;

typedef struct rtl_socket_addr_list {
	rtl_socket_addr_t addr;
	struct rtl_socket_addr_list *next;
} rtl_socket_addr_list_t;

typedef struct rtl_socket_connection {
	int fd;
	int type;
	struct rtl_socket_addr local;
	struct rtl_socket_addr remote;
} rtl_socket_connection_t;

/* socket tcp apis */
struct rtl_socket_connection *rtl_socket_tcp_connect(const char *host, uint16_t port);
void rtl_socket_connection_close(struct rtl_socket_connection *sc);
int rtl_socket_tcp_bind_listen(const char *host, uint16_t port);
int rtl_socket_accept(int fd, uint32_t *ip, uint16_t *port);

/* socket udp apis */
struct rtl_socket_connection *rtl_socket_udp_connect(const char *host, uint16_t port);
int rtl_socket_udp_bind(const char *host, uint16_t port);

/* socket common apis */
void rtl_socket_close(int fd);

int rtl_socket_send(int fd, const void *buf, size_t len);
int rtl_socket_sendto(int fd, const char *ip, uint16_t port,
		const void *buf, size_t len);
int rtl_socket_recv(int fd, void *buf, size_t len);
int rtl_socket_recvfrom(int fd, uint32_t *ip, uint16_t *port,
		void *buf, size_t len);

uint32_t rtl_socket_addr_pton(const char *ip);
int rtl_socket_addr_ntop(char *str, uint32_t ip);

int rtl_socket_set_block(int fd);
int rtl_socket_set_nonblock(int fd);
int rtl_socket_set_reuse(int fd, int enable);
int rtl_socket_set_tcp_keepalive(int fd, int enable);
int rtl_socket_set_buflen(int fd, int len);

int rtl_socket_get_tcp_info(int fd, struct tcp_info *ti);
int rtl_socket_get_local_list(struct rtl_socket_addr_list **list, int loopback);
int rtl_socket_gethostbyname(struct rtl_socket_addr_list **list, const char *name);
int rtl_socket_getaddrinfo(rtl_socket_addr_list_t **list,
		const char *domain, const char *port);
void rtl_socket_free_addr_list(rtl_socket_addr_list_t *al);
int rtl_socket_getaddr_by_fd(int fd, struct rtl_socket_addr *addr);
int rtl_socket_get_remote_addr(struct rtl_socket_addr *addr, int fd);
int rtl_socket_get_local_info(void);

#endif /* _RTL_SOCKET_H_ */
