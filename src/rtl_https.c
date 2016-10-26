#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rtl_http.h"
#include "rtl_https.h"
#include "rtl_socket.h"

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

	ssl->sockfd = rtl_socket_connect(host, port);
	if (ssl->sockfd < 0)
		goto free_ssl;

	if (SSL_set_fd(ssl->ssl, ssl->sockfd) == 0)
		goto free_sockfd;

	if (SSL_connect(ssl->ssl) != 1)
		goto free_all;

	if (ssl_writen(ssl->ssl, header, header_len) < 0)
		goto free_all;

	return 0;

free_all:
	SSL_shutdown(ssl->ssl);
free_sockfd:
	close(ssl->sockfd);
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

	ssl->sockfd = rtl_socket_connect(host, port);
	if (ssl->sockfd < 0)
		goto free_ssl;

	if (SSL_set_fd(ssl->ssl, ssl->sockfd) == 0)
		goto free_sockfd;

	if (SSL_connect(ssl->ssl) != 1)
		goto free_all;

	if (ssl_writen(ssl->ssl, header, header_len) < 0)
		goto free_all;

	if (ssl_writen(ssl->ssl, body, body_len) < 0)
		goto free_all;

	return -1;

free_all:
	SSL_shutdown(ssl->ssl);
free_sockfd:
	close(ssl->sockfd);
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
	close(ssl->sockfd);
	SSL_free(ssl->ssl);
	SSL_CTX_free(ssl->ctx);

	return ret;
}

static int fwriten(uint8_t *buf, int len, FILE *fp)
{
	int n = fwrite(buf, sizeof(uint8_t), len, fp);

	if (n != len)
		return -1;

	return 0;
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
	if (fwriten(buff + body_pos, n - body_pos, fp) < 0)
		goto out;

	for (;;) {
		n = SSL_read(ssl->ssl, buff, sizeof(buff));
		if (n > 0) {
			if (fwriten(buff, n, fp) < 0)
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
	close(ssl->sockfd);
	SSL_free(ssl->ssl);
	SSL_CTX_free(ssl->ctx);

	return ret;
}
