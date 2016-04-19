#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdint.h>

enum request { GET, POST };

int http_build_get_header(const char *hostname, const char *path, char *header);
int http_build_post_header(const char *hostname, const char *path, char *header);
int http_get_body_pos(const char *buff, int size);
int http_send_request(enum request type, const char *host, uint16_t port, const char *path, char *resp, int len);

#endif /* _HTTP_H_ */
