#ifndef _RTL_INET_H_
#define _RTL_INET_H_

#include <stdio.h>

int rtl_get_mac(char *mac, const char *fmt, size_t size, const char *ifname);
int rtl_get_ip(char *ip, size_t size, const char *ifname);
int rtl_get_ssid(char *ssid, size_t size, const char *ifname);
int rtl_mac_str_to_hex(const char *str, unsigned char *mac, size_t size);

#endif /* _RTL_INET_H_ */
