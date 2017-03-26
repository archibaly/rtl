#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rtl_http.h"
#include "rtl_https.h"

static int ssl_writen(SSL *ssl, const void *buf, int num)
{
	int nleft = num;
	int nwritten;

	while (nleft > 0) {
		nwritten = SSL_write(ssl, buf, nleft);
		if (nwritten <= 0)
			break;
		nleft -= nwritten;
		buf   += nwritten;
	}

	return num - nleft;
}

int rtl_https_send_get_request(struct ssl *ssl, const char *path,
							   const char *host, int port)
{
	char header[BUFSIZ];
	int header_len;

	header_len = rtl_http_build_get_header(header, sizeof(header), path, host);

	SSL_library_init();
	ssl->ctx = SSL_CTX_new(SSLv23_client_method());
	if (ssl->ctx == NULL)
		return -1;

	if (!(ssl->ssl = SSL_new(ssl->ctx)))
		goto free_ssl_ctx;

	ssl->sc = rtl_socket_tcp_connect(host, port);
	if (!ssl->sc)
		goto free_ssl;

	if (SSL_set_fd(ssl->ssl, ssl->sc->fd) == 0)
		goto free_sockfd;

	if (SSL_connect(ssl->ssl) != 1)
		goto free_all;

	if (ssl_writen(ssl->ssl, header, header_len) < 0)
		goto free_all;

	return 0;

free_all:
	SSL_shutdown(ssl->ssl);
free_sockfd:
	rtl_socket_connection_close(ssl->sc);
free_ssl:
	SSL_free(ssl->ssl);
free_ssl_ctx:
	SSL_CTX_free(ssl->ctx);

	return -1;
}

int rtl_https_send_post_request(struct ssl *ssl, const char *path,
								const char *host, int port, const uint8_t *body,
								size_t body_len)
{
	char header[BUFSIZ];
	int header_len;

	header_len = rtl_http_build_get_header(header, sizeof(header), path, host);

	SSL_library_init();
	ssl->ctx = SSL_CTX_new(SSLv23_client_method());
	if (ssl->ctx == NULL)
		return -1;

	if (!(ssl->ssl = SSL_new(ssl->ctx)))
		goto free_ssl_ctx;

	ssl->sc = rtl_socket_tcp_connect(host, port);
	if (!ssl->sc)
		goto free_ssl;

	if (SSL_set_fd(ssl->ssl, ssl->sc->fd) == 0)
		goto free_sockfd;

	if (SSL_connect(ssl->ssl) != 1)
		goto free_all;

	if (ssl_writen(ssl->ssl, header, header_len) < 0)
		goto free_all;

	if (ssl_writen(ssl->ssl, body, body_len) < 0)
		goto free_all;

	return 0;

free_all:
	SSL_shutdown(ssl->ssl);
free_sockfd:
	rtl_socket_connection_close(ssl->sc);
free_ssl:
	SSL_free(ssl->ssl);
free_ssl_ctx:
	SSL_CTX_free(ssl->ctx);

	return -1;
}

int rtl_https_recv_response(struct ssl *ssl, uint8_t *resp, size_t size)
{
	int nread;
	int ret = -1;
	uint8_t *ptr = resp;

	for (;;) {
		nread = SSL_read(ssl->ssl, ptr, size);
		if (nread == 0)	/* receive done */
			break;
		else if (nread < 0)
			goto out;
		size -= nread;
		ptr  += nread;
	}

	ret = ptr - resp;

out:
	SSL_shutdown(ssl->ssl);
	rtl_socket_connection_close(ssl->sc);
	SSL_free(ssl->ssl);
	SSL_CTX_free(ssl->ctx);

	return ret;
}

int rtl_https_save_body_to_file(struct ssl *ssl, const char *filename)
{
	int n, ret = -1;
	uint8_t buff[BUFSIZ];

	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		goto out;

	n = SSL_read(ssl->ssl, buff, sizeof(buff));
	if (n < 0)
		goto out;

	int body_pos = rtl_http_get_body_pos(buff, n);
	if (fwrite(buff + body_pos, sizeof(uint8_t), n - body_pos, fp) != n - body_pos)
		goto out;

	for (;;) {
		n = SSL_read(ssl->ssl, buff, sizeof(buff));
		if (n > 0) {
			if (fwrite(buff, sizeof(uint8_t), n, fp) != n)
				goto out;
		} else if (n == 0) {	/* receive done */
			break;
		} else {
			goto out;
		}
	}

	ret = 0;

out:
	if (fp != NULL)
		fclose(fp);
	SSL_shutdown(ssl->ssl);
	rtl_socket_connection_close(ssl->sc);
	SSL_free(ssl->ssl);
	SSL_CTX_free(ssl->ctx);

	return ret;
}

void rtl_https_end_request(struct ssl *ssl)
{
	SSL_shutdown(ssl->ssl);
	rtl_socket_connection_close(ssl->sc);
	SSL_free(ssl->ssl);
	SSL_CTX_free(ssl->ctx);
}
