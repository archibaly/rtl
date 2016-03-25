#ifndef _HTTP_H_
#define _HTTP_H_

int http_build_get_header(const char *hostname, const char *path, char *header);
int http_build_post_header(const char *hostname, const char *path, char *header);
int http_get_body_pos(const char *buff, int size);

#endif /* _HTTP_H_ */
