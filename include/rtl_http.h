#ifndef _RTL_HTTP_H_
#define _RTL_HTTP_H_

#include <stdint.h>

#include "rtl_socket.h"

#define rtl_http_connection	rtl_socket_connection

int rtl_http_build_get_header(char *header, size_t size, const char *path,
							  const char *hostname, int keepalive);
int rtl_http_build_post_header(char *header, size_t size, const char *path,
							   const char *hostname, int content_len, int keepalive);
int rtl_http_get_body_pos(const uint8_t *buff, size_t size);
struct rtl_http_connection *rtl_http_send_get_request(const char *path, const char *host, uint16_t port, int keepalive);
struct rtl_http_connection *rtl_http_send_post_request(const char *path, const char *host, uint16_t port,
													     const uint8_t *body, size_t body_len, int keepalive);
int rtl_http_recv_response(struct rtl_http_connection *hc, uint8_t *resp, size_t size);
int rtl_http_save_body_to_file(struct rtl_http_connection *hc, const char *filename);
void rtl_http_close_connection(struct rtl_http_connection *hc);

#endif /* _RTL_HTTP_H_ */
