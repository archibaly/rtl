#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "rtl_socket.h"

#define MTU					(1500 - 42 - 200)
#define MAX_RETRY_CNT		3
#define MAX_ADDR_LEN		65

#define USE_IPV6			0

static struct rtl_socket_connection *_rtl_socket_connect(int type,
		const char *host, uint16_t port)
{
	int domain = AF_INET;
	int protocol = 0;
	struct sockaddr_in si;
	struct rtl_socket_connection *sc = NULL;

	if (type < 0 || strlen(host) == 0 || port >= 0xFFFF) {
		fprintf(stderr, "invalid paraments\n");
		return NULL;
	}
	sc = calloc(1, sizeof(struct rtl_socket_connection));
	if (sc == NULL) {
		fprintf(stderr, "malloc failed!\n");
		return NULL;
	}
	sc->fd = socket(domain, type, protocol);
	if (-1 == sc->fd) {
		fprintf(stderr, "socket: %s\n", strerror(errno));
		goto fail;
	}

	si.sin_family = domain;
	si.sin_addr.s_addr = inet_addr(host);
	si.sin_port = htons(port);
	if (-1 == connect(sc->fd, (struct sockaddr*)&si, sizeof(si))) {
		fprintf(stderr, "connect failed: %s\n", strerror(errno));
		goto fail;
	}
	sc->remote.ip = inet_addr(host);
	sc->remote.port = port;
	sc->type = type;
	if (-1 == rtl_socket_getaddr_by_fd(sc->fd, &sc->local)) {
		fprintf(stderr, "rtl_socket_getaddr_by_fd failed: %s\n", strerror(errno));
		goto fail;

	}

	return sc;
fail:
	if (-1 != sc->fd) {
		close(sc->fd);
	}
	if (sc) {
		free(sc);
	}
	return NULL;
}

static int is_ipv4(const char *str)
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


struct rtl_socket_connection *rtl_socket_tcp_connect(const char *host, uint16_t port)
{
	char ip[INET_ADDRSTRLEN];
	rtl_socket_addr_list_t *list;

	if (!is_ipv4(host)) {
		if (rtl_socket_getaddrinfo(&list, host, NULL) < 0)
			return NULL;
		rtl_socket_addr_ntop(ip, list->addr.ip);
		rtl_socket_free_addr_list(list);
	} else {
		strncpy(ip, host, sizeof(INET_ADDRSTRLEN));
	}
	return _rtl_socket_connect(SOCK_STREAM, ip, port);
}

struct rtl_socket_connection *rtl_socket_udp_connect(const char *host, uint16_t port)
{
	char ip[INET_ADDRSTRLEN];
	rtl_socket_addr_list_t *list;

	if (!is_ipv4(host)) {
		if (rtl_socket_getaddrinfo(&list, host, NULL) < 0)
			return NULL;
		rtl_socket_addr_ntop(ip, list->addr.ip);
		rtl_socket_free_addr_list(list);
	} else {
		strncpy(ip, host, sizeof(INET_ADDRSTRLEN));
	}
	return _rtl_socket_connect(SOCK_DGRAM, ip, port);
}

void rtl_socket_connection_close(struct rtl_socket_connection *sc)
{
	close(sc->fd);
	free(sc);
}

int rtl_socket_tcp_bind_listen(const char *host, uint16_t port)
{
	struct sockaddr_in si;
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == fd) {
		fprintf(stderr, "socket failed: %s\n", strerror(errno));
		goto fail;
	}
	if (-1 == rtl_socket_set_reuse(fd, 1)) {
		fprintf(stderr, "rtl_socket_set_reuse failed: %s\n", strerror(errno));
		goto fail;
	}
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
	si.sin_port = htons(port);
	if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
		fprintf(stderr, "bind failed: %s\n", strerror(errno));
		goto fail;
	}
	if (-1 == listen(fd, SOMAXCONN)) {
		fprintf(stderr, "listen failed: %s\n", strerror(errno));
		goto fail;
	}
	return fd;
fail:
	if (-1 != fd) {
		close(fd);
	}
	return -1;
}

int rtl_socket_udp_bind(const char *host, uint16_t port)
{
	int fd;
	struct sockaddr_in si;

	si.sin_family = AF_INET;
	si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
	si.sin_port = htons(port);
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == fd) {
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return -1;
	}
	if (-1 == rtl_socket_set_reuse(fd, 1)) {
		fprintf(stderr, "rtl_socket_set_reuse failed!\n");
		close(fd);
		return -1;
	}
	if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
		fprintf(stderr, "bind: %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

int rtl_socket_accept(int fd, uint32_t *ip, uint16_t *port)
{
	struct sockaddr_in si;
	socklen_t len = sizeof(si);
	int afd = accept(fd, (struct sockaddr *)&si, &len);
	if (afd == -1) {
		fprintf(stderr, "accept: %s\n", strerror(errno));
		return -1;
	} else {
		*ip = si.sin_addr.s_addr;
		*port = ntohs(si.sin_port);
	}
	return afd;
}

void rtl_socket_close(int fd)
{
	close(fd);
}

int rtl_socket_get_tcp_info(int fd, struct tcp_info *tcpi)
{
	socklen_t len = sizeof(*tcpi);
	return getsockopt(fd, SOL_TCP, TCP_INFO, tcpi, &len);
}

int rtl_socket_get_local_list(rtl_socket_addr_list_t **al, int loopback)
{
	struct ifaddrs *ifs = NULL;
	struct ifaddrs *ifa = NULL;
	rtl_socket_addr_list_t *ap, *an;

	if (getifaddrs(&ifs) < 0) {
		fprintf(stderr, "getifaddrs: %s\n", strerror(errno));
		return -1;
	}

	ap = NULL;
	*al = NULL;
	for (ifa = ifs; ifa != NULL; ifa = ifa->ifa_next) {

		char saddr[MAX_ADDR_LEN] = "";
		if (!(ifa->ifa_flags & IFF_UP))
			continue;
		if (!(ifa->ifa_addr))
			continue;
		if (ifa ->ifa_addr->sa_family == AF_INET) {
			if (!inet_ntop(AF_INET,
						&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
						saddr, INET_ADDRSTRLEN))
				continue;
			if (strstr(saddr,"169.254.") == saddr)
				continue;
			if (!strcmp(saddr,"0.0.0.0"))
				continue;
		} else if (ifa->ifa_addr->sa_family == AF_INET6) {
			if (!inet_ntop(AF_INET6,
						&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,
						saddr, INET6_ADDRSTRLEN))
				continue;
			if (strstr(saddr,"fe80") == saddr)
				continue;
			if (!strcmp(saddr,"::"))
				continue;
		} else {
			continue;
		}
		if ((ifa->ifa_flags & IFF_LOOPBACK) && !loopback)
			continue;

		an = (rtl_socket_addr_list_t *)calloc(sizeof(rtl_socket_addr_list_t), 1);
		an->addr.ip = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
		an->addr.port = 0;
		an->next = NULL;
		if (*al == NULL) {
			*al = an;
			ap = *al;
		} else {
			ap->next = an;
			ap = ap->next;
		}
	}
	freeifaddrs(ifs);
	return 0;
}

int rtl_socket_get_remote_addr(struct rtl_socket_addr *addr, int fd)
{
	struct sockaddr_in si;
	socklen_t len = sizeof(si);

	if (-1 == getpeername(fd, (struct sockaddr *)&(si.sin_addr), &len)) {
		fprintf(stderr, "getpeername: %s\n", strerror(errno));
		return -1;
	}
	addr->ip = si.sin_addr.s_addr;
	addr->port = ntohs(si.sin_port);
	return 0;
}

int rtl_socket_getaddr_by_fd(int fd, struct rtl_socket_addr *addr)
{
	struct sockaddr_in si;
	socklen_t len = sizeof(si);
	memset(&si, 0, len);

	if (-1 == getsockname(fd, (struct sockaddr *)&si, &len)) {
		fprintf(stderr, "getsockname: %s\n", strerror(errno));
		return -1;
	}

	addr->ip = si.sin_addr.s_addr;
	addr->port = ntohs(si.sin_port);

	return 0;
}

uint32_t rtl_socket_addr_pton(const char *ip)
{
	struct in_addr ia;
	int ret;

	ret = inet_pton(AF_INET, ip, &ia);
	if (ret == -1) {
		fprintf(stderr, "inet_pton: %s\n", strerror(errno));
		return -1;
	} else if (ret == 0) {
		fprintf(stderr, "inet_pton not in presentation format\n");
		return -1;
	}
	return ia.s_addr;
}

int rtl_socket_addr_ntop(char *str, uint32_t ip)
{
	struct in_addr ia;
	char tmp[MAX_ADDR_LEN];

	ia.s_addr = ip;
	if (NULL == inet_ntop(AF_INET, &ia, tmp, INET_ADDRSTRLEN)) {
		fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
		return -1;
	}
	strncpy(str, tmp, INET_ADDRSTRLEN);
	return 0;
}

int rtl_socket_getaddrinfo(rtl_socket_addr_list_t **al, const char *domain, const char *port)
{
	int ret;
	struct addrinfo hints, *ai_list, *rp;
	rtl_socket_addr_list_t *ap, *an;

	memset(&hints, 0, sizeof(struct addrinfo));
#if USE_IPV6
	hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
#else
	hints.ai_family = AF_INET;   /* Allows IPv4 or IPv6 */
#endif
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_CANONNAME;

	ret = getaddrinfo(domain, port, &hints, &ai_list);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	ap = NULL;
	*al = NULL;
	for (rp = ai_list; rp != NULL; rp = rp->ai_next ) {
		an = (rtl_socket_addr_list_t *)calloc(sizeof(rtl_socket_addr_list_t), 1);
		an->addr.ip = ((struct sockaddr_in *)rp->ai_addr)->sin_addr.s_addr;
		an->addr.port = ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port);
		an->next = NULL;
		if (*al == NULL) {
			*al = an;
			ap = *al;
		} else {
			ap->next = an;
			ap = ap->next;
		}
	}
	if (al == NULL) {
		fprintf(stderr, "Could not get ip address of %s!\n", domain);
		return -1;
	}
	freeaddrinfo(ai_list);
	return 0;
}

int rtl_socket_gethostbyname(rtl_socket_addr_list_t **al, const char *name)
{
	rtl_socket_addr_list_t *ap, *an;
	struct hostent *host;
	char **p;

	host = gethostbyname(name);
	if (NULL == host) {
		herror("gethostbyname failed!\n");
		return -1;
	}
	if (host->h_addrtype != AF_INET && host->h_addrtype != AF_INET6) {
		herror("addrtype error!\n");
		return -1;
	}

	ap = *al = NULL;
	for (p = host->h_addr_list; *p != NULL; p++) {
		an = (rtl_socket_addr_list_t *)calloc(sizeof(rtl_socket_addr_list_t), 1);
		an->addr.ip = ((struct in_addr*)(*p))->s_addr;
		an->addr.port = 0;
		an->next = NULL;
		if (*al == NULL) {
			*al = an;
			ap = *al;
		} else {
			ap->next = an;
			ap = ap->next;
		}
		if (al == NULL) {
			fprintf(stderr, "Could not get ip address of %s!\n", name);
			return -1;
		}
	}
	return 0;
}

void rtl_socket_free_addr_list(rtl_socket_addr_list_t *al)
{
	if (!al)
		return;

	rtl_socket_addr_list_t *tmp;

	for (tmp = al->next; al; tmp = tmp->next) {
		free(al);
		al = tmp;
		if (!tmp)
			break;
	}
}

int rtl_socket_get_local_info(void)
{
	int fd;
	int interfaceNum = 0;
	struct ifreq buf[16];
	struct ifconf ifc;
	struct ifreq ifrcopy;
	char mac[16] = {0};
	char ip[32] = {0};
	char broadAddr[32] = {0};
	char subnetMask[32] = {0};

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		close(fd);
		return -1;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t)buf;
	if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
		interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
		printf("interface num = %d\n", interfaceNum);
		while (interfaceNum-- > 0) {
			printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

			/* ignore the interface that not up or not runing */
			ifrcopy = buf[interfaceNum];
			if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)) {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(fd);
				return -1;
			}

			/* get the mac of this interface */
			if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum]))) {
				memset(mac, 0, sizeof(mac));
				snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
						(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
				printf("device mac: %s\n", mac);
			} else {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(fd);
				return -1;
			}

			/* get the IP of this interface */
			if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum])) {
				snprintf(ip, sizeof(ip), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
				printf("device ip: %s\n", ip);
			} else {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(fd);
				return -1;
			}

			/* get the broad address of this interface */
			if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum])) {
				snprintf(broadAddr, sizeof(broadAddr), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
				printf("device broadAddr: %s\n", broadAddr);
			} else {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(fd);
				return -1;
			}

			/* get the subnet mask of this interface */
			if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum])) {
				snprintf(subnetMask, sizeof(subnetMask), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
				printf("device subnetMask: %s\n", subnetMask);
			} else {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(fd);
				return -1;
			}
		}
	} else {
		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int rtl_socket_set_block(int fd)
{
	int flag;
	flag = fcntl(fd, F_GETFL);
	if (flag == -1) {
		fprintf(stderr, "fcntl: %s\n", strerror(errno));
		return -1;
	}
	flag &= ~O_NONBLOCK;
	if (-1 == fcntl(fd, F_SETFL, flag)) {
		fprintf(stderr, "fcntl: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int rtl_socket_set_nonblock(int fd)
{
	int flag;
	flag = fcntl(fd, F_GETFL);
	if (flag == -1) {
		fprintf(stderr, "fcntl: %s\n", strerror(errno));
		return -1;
	}
	flag |= O_NONBLOCK;
	if (-1 == fcntl(fd, F_SETFL, flag)) {
		fprintf(stderr, "fcntl: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
int rtl_socket_set_reuse(int fd, int enable)
{
	int on = !!enable;

#ifdef SO_REUSEPORT
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
		fprintf(stderr, "setsockopt SO_REUSEPORT: %s\n", strerror(errno));
		return -1;
	}
#endif
#ifdef SO_REUSEADDR
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		fprintf(stderr, "setsockopt SO_REUSEADDR: %s\n", strerror(errno));
		return -1;
	}
#endif
	return 0;
}

int rtl_socket_set_tcp_keepalive(int fd, int enable)
{
	int on = !!enable;

#ifdef SO_KEEPALIVE
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
				(const void*)&on, (socklen_t) sizeof(on))) {
		fprintf(stderr, "setsockopt SO_KEEPALIVE: %s\n", strerror(errno));
		return -1;
	}
#endif
	return 0;
}

int rtl_socket_set_buflen(int fd, int size)
{
	int sz;

	sz = size;
	while (sz > 0) {
		if (-1 == setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
					(const void*) (&sz), (socklen_t) sizeof(sz))) {
			sz = sz / 2;
		} else {
			break;
		}
	}

	if (sz < 1) {
		fprintf(stderr, "setsockopt SO_RCVBUF: %s\n", strerror(errno));
	}

	sz = size;
	while (sz > 0) {
		if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
					(const void*) (&sz), (socklen_t) sizeof(sz))) {
			sz = sz / 2;
		} else {
			break;
		}
	}

	if (sz < 1) {
		fprintf(stderr, "setsockopt SO_SNDBUF: %s\n", strerror(errno));
	}
	return 0;
}

int rtl_socket_send(int fd, const void *buf, size_t len)
{
	ssize_t n;
	char *p = (char *)buf;
	size_t left = len;
	size_t step = MTU;
	int cnt = 0;

	if (buf == NULL || len == 0) {
		fprintf(stderr, "%s paraments invalid!\n", __func__);
		return -1;
	}

	while (left > 0) {
		if (left < step)
			step = left;
		if ((n = send(fd, p, step, 0)) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				if (++cnt > MAX_RETRY_CNT)
					break;
				continue;
			} else
				return -1;
		} else if (n == 0) {
			break;
		}
		left -= n;
		p += n;
	}
	return len - left;
}

int rtl_socket_sendto(int fd, const char *ip, uint16_t port,
		const void *buf, size_t len)
{
	ssize_t n;
	char *p = (char *)buf;
	size_t left = len;
	size_t step = MTU;
	struct sockaddr_in sa;
	int cnt = 0;

	if (buf == NULL || len == 0) {
		fprintf(stderr, "%s paraments invalid!\n", __func__);
		return -1;
	}
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ip);
	sa.sin_port = htons(port);

	while (left > 0) {
		if (left < step)
			step = left;
		if ((n = sendto(fd, p, step, 0, (struct sockaddr*)&sa, sizeof(sa))) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				if (++cnt > MAX_RETRY_CNT)
					break;
				continue;
			} else
				return -1;
		} else if (n == 0) {
			break;
		}
		left -= n;
		p += n;
	}
	return len - left;
}

int rtl_socket_recv(int fd, void *buf, size_t len)
{
	int n;
	char *p = (char *)buf;
	size_t left = len;
	size_t step = MTU;
	int cnt = 0;

	if (buf == NULL || len == 0) {
		fprintf(stderr, "%s paraments invalid!\n", __func__);
		return -1;
	}
	while (left > 0) {
		if (left < step)
			step = left;
		if ((n = recv(fd, p, step, 0)) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				if (++cnt > MAX_RETRY_CNT)
					break;
				continue;
			} else
				return -1;
		} else if (n == 0) {
			break;			/* peer closed */
		}
		left -= n;
		p += n;
	}
	return len - left;
}

int rtl_socket_send_sync_recv(int fd, const void *sbuf, size_t slen,
		void *rbuf, size_t rlen, int timeout)
{
	rtl_socket_send(fd, sbuf, slen);
	rtl_socket_set_nonblock(fd);
	rtl_socket_recv(fd, rbuf, rlen);

	return 0;
}

int rtl_socket_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
	int n;
	char *p = (char *)buf;
	int cnt = 0;
	size_t left = len;
	size_t step = MTU;
	struct sockaddr_in si;
	socklen_t si_len = sizeof(si);

	memset(&si, 0, sizeof(si));

	while (left > 0) {
		if (left < step)
			step = left;
		if ((n = recvfrom(fd, p, step, 0, (struct sockaddr *)&si, &si_len)) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				if (++cnt > MAX_RETRY_CNT)
					break;
				continue;
			} else
				return -1;
		} else if (n == 0) {
			break;			/* peer closed */
		}
		left -= n;
		p += n;
	}

	*ip = si.sin_addr.s_addr;
	*port = ntohs(si.sin_port);

	return len - left;
}
