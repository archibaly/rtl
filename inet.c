#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "inet.h"
#include "debug.h"

int get_mac(char *mac, const char *type)
{
	if (NULL == mac)
		return -1;
	struct ifreq _ifreq;
	int sock = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ERROR("socket()");
		return -1;
	}
	strcpy(_ifreq.ifr_name, type);
	if(ioctl(sock, SIOCGIFHWADDR, &_ifreq) < 0) {
		ERROR("ioctl()");
		return -1;
	}

	sprintf(mac, "%02X%02X%02X%02X%02X%02X",
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[0], (unsigned char)_ifreq.ifr_hwaddr.sa_data[1],
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[2], (unsigned char)_ifreq.ifr_hwaddr.sa_data[3],
			(unsigned char)_ifreq.ifr_hwaddr.sa_data[4], (unsigned char)_ifreq.ifr_hwaddr.sa_data[5]);

	close(sock);

	return 0;
}

int get_ip(char *ip, const char *type)
{
	int sock;
	struct ifreq ifr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0) < 0)) {
		ERROR("socket()");
		return -1;
	}

	strcpy(ifr.ifr_name, type);
	if (ioctl(sock, SIOCGIFADDR, &ifr) <  0) {
		ERROR("ioctl()");
		return -1;
	}
	sprintf(ip, "%s", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));

	close(sock);

	return 0;
}
