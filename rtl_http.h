#ifndef _RTL_HTTP_H_
#define _RTL_HTTP_H_

#include <stdint.h>

#define RTL_HTTP_GET	1
#define RTL_HTTP_POST	2

int rtl_http_build_get_header(const char *hostname, const char *path, char *header);
int rtl_http_build_post_header(const char *hostname, const char *path, char *header);
int rtl_http_get_body_pos(const char *buff, int size);
int rtl_http_send_request(int type, const char *host, uint16_t port, const char *path, char *resp, int len);

#endif /* _RTL_HTTP_H_ */
