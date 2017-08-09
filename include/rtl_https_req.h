#ifndef _RTL_HTTPS_REQ_H_
#define _RTL_HTTPS_REQ_H_

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "rtl_socket.h"
#include "rtl_http_req.h"

struct rtl_https_connection {
	SSL *ssl;
	SSL_CTX *ctx;
	struct rtl_socket_connection *sc;
};

struct rtl_https_connection *rtl_https_req_conn(rtl_http_req_t *req);
void rtl_https_req_discon(struct rtl_https_connection *conn);
int rtl_https_req_send(const rtl_http_req_t *req,
					   struct rtl_https_connection *conn,
					   const unsigned char *body, int body_len);

#endif /* _RTL_HTTPS_REQ_H_ */
