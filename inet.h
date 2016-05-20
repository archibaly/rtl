#ifndef _INET_H_
#define _INET_H_

int mac_format_valid(char *mac);
int ip_format_valid(char *ip);
int get_mac(char *mac, size_t size, const char *ifname);
int get_ip(char *ip, size_t size, const char *ifname);

#endif /* _INET_H_ */
