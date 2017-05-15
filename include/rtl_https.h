#ifndef _RTL_HTTPS_H_
#define _RTL_HTTPS_H_

#include <stdint.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "rtl_socket.h"

struct rtl_https_connection {
	SSL *ssl;
	SSL_CTX *ctx;
	struct rtl_socket_connection *sc;
};

struct rtl_https_connection *rtl_https_send_get_request(const char *path, const char *host,
														int port, int keepalive);
struct rtl_https_connection *rtl_https_send_post_request(const char *path, const char *host,
														 int port, const uint8_t *body,
														 size_t body_len, int keepalive);
int rtl_https_recv_response(struct rtl_https_connection *hc, uint8_t *resp, size_t size);
int rtl_https_save_body_to_file(struct rtl_https_connection *hc, const char *filename);
void rtl_https_end_request(struct rtl_https_connection *hc);
void rtl_https_close_connection(struct rtl_https_connection *hc);

#endif /* _RTL_HTTPS_H_ */
