#ifndef _INET_H_
#define _INET_H_

int mac_format_valid(char *possiblemac);
int ip_format_valid(char *possibleip);
int get_mac(char *mac, const char *type);
int get_ip(char *ip, const char *type);

#endif /* _INET_H_ */
