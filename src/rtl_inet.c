#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/wireless.h>

#include "rtl_inet.h"
#include "rtl_debug.h"

#ifndef IF_NAMESIZE
#define IF_NAMESIZE	16
#endif

static int ifctrl_get_ifreq(const char *ifname, struct ifreq *ifreq)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	memset(ifreq, 0, sizeof(struct ifreq));
	strncpy(ifreq->ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFFLAGS, ifreq) < 0) {
		rtl_debug("ioctl error: %s", strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

static int ifctrl_set_ifreq(int fd, struct ifreq *ifreq)
{
	if (ioctl(fd, SIOCSIFFLAGS, ifreq) < 0) {
		rtl_debug("ioctl error: %s", strerror(errno));
		close(fd);
		return -1;
	}

	if (close(fd))
		return -1;

	return 0;
}

static int ifctrl_up(const char *ifname, int up)
{
	int fd;
	struct ifreq ifreq;

	fd = ifctrl_get_ifreq(ifname, &ifreq);
	if (fd < 0)
		return -1;

	if (up)
		ifreq.ifr_flags |= IFF_UP;
	else
		ifreq.ifr_flags &= ~IFF_UP;

	return ifctrl_set_ifreq(fd, &ifreq);
}

static int ifctrl_promisc(const char *ifname, int promisc)
{
	int fd;
	struct ifreq ifreq;

	fd = ifctrl_get_ifreq(ifname, &ifreq);
	if (fd < 0)
		return -1;

	if (promisc)
		ifreq.ifr_flags |= IFF_PROMISC;
	else
		ifreq.ifr_flags &= ~IFF_PROMISC;

	return ifctrl_set_ifreq(fd, &ifreq);
}

int rtl_if_up(const char *ifname)
{
	return ifctrl_up(ifname, 1);
}

int rtl_if_down(const char *ifname)
{
	return ifctrl_up(ifname, 0);
}

int rtl_enable_promisc(const char *ifname)
{
	return ifctrl_promisc(ifname, 1);
}

int rtl_disable_promisc(const char *ifname)
{
	return ifctrl_promisc(ifname, 0);
}

int rtl_get_mac(char *mac, const char *fmt, size_t size, const char *ifname)
{
	int sock;
	struct ifreq _ifreq;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		rtl_debug("socket error: %s", strerror(errno));
		return -1;
	}
	strncpy(_ifreq.ifr_name, ifname, sizeof(_ifreq.ifr_name) - 1);
	_ifreq.ifr_name[sizeof(_ifreq.ifr_name) - 1] = '\0';
	if(ioctl(sock, SIOCGIFHWADDR, &_ifreq) < 0) {
		rtl_debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	snprintf(mac, size, fmt,
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[0],
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[1],
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[2],
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[3],
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[4],
			 (unsigned char)_ifreq.ifr_hwaddr.sa_data[5]);

	close(sock);

	return 0;
}

int rtl_get_ip(char *ip, size_t size, const char *ifname)
{
	int sock;
	struct ifreq _ifreq;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		rtl_debug("socket error: %s", strerror(errno));
		return -1;
	}
	strncpy(_ifreq.ifr_name, ifname, sizeof(_ifreq.ifr_name) - 1);
	_ifreq.ifr_name[sizeof(_ifreq.ifr_name) - 1] = '\0';
	if (ioctl(sock, SIOCGIFADDR, &_ifreq) < 0) {
		rtl_debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	strncpy(ip, inet_ntoa(((struct sockaddr_in*)&(_ifreq.ifr_addr))->sin_addr),
			size - 1);
	ip[size - 1] = '\0';

	close(sock);

	return 0;
}

int rtl_get_ssid(char *ssid, size_t size, const char *ifname)
{
	int sock;
	struct iwreq iw;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		rtl_debug("socket error: %s", strerror(errno));
		return -1;
	}

	iw.u.essid.pointer = ssid;
	iw.u.essid.length = size;

	strncpy(iw.ifr_name, ifname, IFNAMSIZ - 1);
	iw.ifr_name[IFNAMSIZ - 1] = '\0';

	if (ioctl(sock, SIOCGIWESSID, &iw) < 0) {
		rtl_debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	close(sock);

	return 0;
}

int rtl_get_mac_from_ip(char *mac, size_t size, const char *ip)
{
	int ret = -1;
	FILE *proc;
	char ip_tmp[16];
	char mac_tmp[18];

	if (!(proc = fopen("/proc/net/arp", "r"))) {
		return -1;
	}

	/* skip first line */
	while (!feof(proc) && fgetc(proc) != '\n')
		;

	while (!feof(proc) && (fscanf(proc, " %15[0-9.] %*s %*s %17[A-Fa-f0-9:] %*s %*s", ip_tmp, mac_tmp) == 2)) {
		if (strcmp(ip_tmp, ip) == 0) {
			strncpy(mac, mac_tmp, size);
			mac[size - 1] = '\0';
			ret = 0;
			break;
		}
	}

	fclose(proc);
	return ret;
}

static int str_to_hex(const char *str, unsigned char *result)
{
	int i;
	int len = strlen(str);

	*result = 0;

	for (i = 0; i < len; i++) {
		if (isdigit(str[i])) {
			*result |= ((str[i] - '0') << ((len - i - 1) << 2));
		} else if (str[i] >= 'a' && str[i] <= 'f') {
			*result |= ((str[i] - 'a' + 10) << ((len - i - 1) << 2));
		} else if (str[i] >= 'A' && str[i] <= 'F') {
			*result |= ((str[i] - 'A' + 10) << ((len - i - 1) << 2));
		} else {
			return -1;
		}
	}

	return 0;
}

/* "xx:xx:xx:xx:xx:xx" -> {xx, xx, xx, xx, xx, xx} */
int rtl_mac_str_to_hex(const char *str, unsigned char *mac, size_t size)
{
	char *cp, *tmp, *p;
	size_t i;

	if (!(cp = strdup(str)))
		return -1;

	tmp = cp;

	for (i = 0; i < size; i++) {
		if ((p = strsep(&cp, ":")) != NULL)
			str_to_hex(p, mac + i);
		else
			break;
	}

	free(tmp);
	return 0;
}

/*
 * 10.x.x.x -> 0xA
 * 172.16.x.x ~ 172.31.x.x -> 0xAC1
 * 192.168.x.x -> 0xC0A8
 */
int rtl_is_inner_ip(const char *ip)
{
	struct in_addr addr;
	uint32_t hostip;

	if (inet_pton(AF_INET, ip, &addr) <= 0)
		return 0;

	hostip = ntohl(addr.s_addr);
	return (hostip >> 24 == 0xA ||
			hostip >> 20 == 0xAC1 ||
			hostip >> 16 == 0xC0A8);
}
