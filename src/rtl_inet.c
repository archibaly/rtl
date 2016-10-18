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

int rtl_get_mac(char *mac, size_t size, const char *ifname)
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

	snprintf(mac, size, "%02X:%02X:%02X:%02X:%02X:%02X",
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
	char *copy;
	char *tmp;

	if (!(copy = strdup(str)))
		return -1;

	tmp = copy;

	char *p;
	size_t i;

	for (i = 0; i < size; i++) {
		if ((p = strsep(&copy, ":")) != NULL)
			str_to_hex(p, mac + i);
		else
			break;
	}

	free(tmp);
	return 0;
}
