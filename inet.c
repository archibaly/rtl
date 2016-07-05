#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/wireless.h>

#include "inet.h"
#include "debug.h"

int mac_fmt_valid(char *mac)
{
	char hex2[3];
	return sscanf(mac,
				  "%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]",
				  hex2, hex2, hex2, hex2, hex2, hex2) == 6;
}

int ip_fmt_valid(char *ip)
{
	char hex3[4];
	return sscanf(ip,
				  "%3[0-9].%3[0-9].%3[0-9].%3[0-9]",
				  hex3, hex3, hex3, hex3) == 4;
}

int get_mac(char *mac, size_t size, const char *ifname)
{
	int sock;
	struct ifreq _ifreq;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		debug("socket error: %s", strerror(errno));
		return -1;
	}
	strlcpy(_ifreq.ifr_name, ifname, sizeof(_ifreq.ifr_name));
	if(ioctl(sock, SIOCGIFHWADDR, &_ifreq) < 0) {
		debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	snprintf(mac, size, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[0], (unsigned char)_ifreq.ifr_hwaddr.sa_data[1],
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[2], (unsigned char)_ifreq.ifr_hwaddr.sa_data[3],
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[4], (unsigned char)_ifreq.ifr_hwaddr.sa_data[5]);

	close(sock);

	return 0;
}

int get_ip(char *ip, size_t size, const char *ifname)
{
	int sock;
	struct ifreq _ifreq;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		debug("socket error: %s", strerror(errno));
		return -1;
	}
	strlcpy(_ifreq.ifr_name, ifname, sizeof(_ifreq.ifr_name));
	if (ioctl(sock, SIOCGIFADDR, &_ifreq) < 0) {
		debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	strlcpy(ip, inet_ntoa(((struct sockaddr_in*)&(_ifreq.ifr_addr))->sin_addr), size);

	close(sock);

	return 0;
}

int get_ssid(char *ssid, size_t size, const char *ifname)
{
	int sock;
	struct iwreq iw; 

	sock = socket(AF_INET, SOCK_DGRAM, 0); 

	if (sock < 0) {
		debug("socket error: %s", strerror(errno));
		return -1; 
	}   

	iw.u.essid.pointer = ssid;
	iw.u.essid.length = size;

	strncpy(iw.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sock, SIOCGIWESSID, &iw) < 0) {
		debug("ioctl error: %s", strerror(errno));
		close(sock);
		return -1;
	}

	close(sock);

	return 0;
}

static int strtohex(const char *str, uint8_t *result)
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
int mac_str_to_hex(const char *str, uint8_t *mac, size_t size)
{
	char *copy;

	if (!(copy = strdup(str)))
		return -1;

	char *p;
	size_t i;
	for (i = 0; i < size; i++) {
		if ((p = strsep(&copy, ":")) != NULL)
			strtohex(p, mac + i);
		else
			break;
	}

	free(copy);
	return 0;
}
