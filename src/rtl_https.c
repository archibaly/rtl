#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "rtl_https.h"
#include "rtl_socket.h"

/*
 * @type: RTL_HTTPGET or RTL_HTTP_POST
 * @host: the remote host
 * @port: the remote host port
 * @path: the path to request
 * @resp: response buffer
 * @len: the length of response buffer
 */
int https_send_request(int type, const char *host, uint16_t port, const char *path, char *resp, int len)
{
	char header[1024];
	int header_len;

	if (type == RTL_HTTP_GET)
		header_len = rtl_http_build_get_header(host, path, header);
	else if (type == RTL_HTTP_POST)
		header_len = rtl_http_build_post_header(host, path, header);
	else
		return -1;

	SSL_library_init();
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
		return -1;

	SSL *ssl = SSL_new(ctx);
	if (ssl == NULL)
		goto free_ssl_ctx;

	int sockfd = rtl_socket_connect(host, port);
	if (sockfd < 0)
		goto free_ssl;

	if (SSL_set_fd(ssl, sockfd) == 0)
		goto free_sockfd;

	if (SSL_connect(ssl) != 1)
		goto free_all;

	int nleft = header_len;
	int nwritten;
	char *ptr = header;
	while (nleft > 0) {
		if ((nwritten = SSL_write(ssl, ptr, nleft)) <= 0) {
			goto free_all;
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}

	/* read response */
	int nread;
	ptr = resp;
	for (;;) {
		nread = SSL_read(ssl, ptr, len);
		if (nread == 0) {	/* receive done */
			break;
		} else if (nread < 0) {
			goto free_all;
		}
		len -= nread;
		ptr += nread;
		if (len < 0)	/* the buffer of resp is not enough */
			goto free_all;
	}
	return ptr - resp;

free_ssl_ctx:
	SSL_CTX_free(ctx);
	return -1;

free_ssl:
	SSL_CTX_free(ctx);
	SSL_free(ssl);
	return -1;

free_sockfd:
	SSL_CTX_free(ctx);
	SSL_free(ssl);
	close(sockfd);
	return -1;

free_all:
	SSL_CTX_free(ctx);
	SSL_free(ssl);
	close(sockfd);
	SSL_shutdown(ssl);
	return -1;
}
