#ifndef _INET_H_
#define _INET_H_

int mac_fmt_valid(char *mac);
int ip_fmt_valid(char *ip);
int get_mac(char *mac, size_t size, const char *ifname);
int get_ip(char *ip, size_t size, const char *ifname);
int get_ssid(char *ssid, size_t size, const char *ifname);
int mac_str_to_hex(const char *str, unsigned char *mac, size_t size);

#endif /* _INET_H_ */
