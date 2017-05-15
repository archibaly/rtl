#include <stdio.h>
#include <stdlib.h>
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

struct rtl_https_connection *rtl_https_send_get_request(const char *path, const char *host,
														int port, int keepalive)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_https_connection *hc;

	if (!(hc = malloc(sizeof(struct rtl_https_connection))))
		return NULL;

	header_len = rtl_http_build_get_header(header, sizeof(header), path, host, keepalive);

	SSL_library_init();
	hc->ctx = SSL_CTX_new(SSLv23_client_method());
	if (hc->ctx == NULL)
		goto free_hc;

	if (!(hc->ssl = SSL_new(hc->ctx)))
		goto free_ssl_ctx;

	hc->sc = rtl_socket_tcp_connect(host, port);
	if (!hc->sc)
		goto free_ssl;

	if (SSL_set_fd(hc->ssl, hc->sc->fd) == 0)
		goto free_sockfd;

	if (SSL_connect(hc->ssl) != 1)
		goto free_all;

	if (ssl_writen(hc->ssl, header, header_len) < 0)
		goto free_all;

	return hc;

free_all:
	SSL_shutdown(hc->ssl);
free_sockfd:
	rtl_socket_connection_close(hc->sc);
free_ssl:
	SSL_free(hc->ssl);
free_ssl_ctx:
	SSL_CTX_free(hc->ctx);
free_hc:
	free(hc);

	return NULL;
}

struct rtl_https_connection *rtl_https_send_post_request(const char *path, const char *host,
														 int port, const uint8_t *body,
														 size_t body_len, int keepalive)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_https_connection *hc;

	if (!(hc = malloc(sizeof(struct rtl_https_connection))))
		return NULL;

	header_len = rtl_http_build_post_header(header, sizeof(header), path, host,
											body_len, keepalive);

	SSL_library_init();
	hc->ctx = SSL_CTX_new(SSLv23_client_method());
	if (hc->ctx == NULL)
		goto free_hc;

	if (!(hc->ssl = SSL_new(hc->ctx)))
		goto free_ssl_ctx;

	hc->sc = rtl_socket_tcp_connect(host, port);
	if (!hc->sc)
		goto free_ssl;

	if (SSL_set_fd(hc->ssl, hc->sc->fd) == 0)
		goto free_sockfd;

	if (SSL_connect(hc->ssl) != 1)
		goto free_all;

	if (ssl_writen(hc->ssl, header, header_len) < 0)
		goto free_all;

	if (ssl_writen(hc->ssl, body, body_len) < 0)
		goto free_all;

	return hc;

free_all:
	SSL_shutdown(hc->ssl);
free_sockfd:
	rtl_socket_connection_close(hc->sc);
free_ssl:
	SSL_free(hc->ssl);
free_ssl_ctx:
	SSL_CTX_free(hc->ctx);
free_hc:
	free(hc);

	return NULL;
}

int rtl_https_recv_response(struct rtl_https_connection *hc, uint8_t *resp, size_t size)
{
	return SSL_read(hc->ssl, resp, size);
}

int rtl_https_save_body_to_file(struct rtl_https_connection *hc, const char *filename)
{
	int n, ret = -1;
	uint8_t buff[BUFSIZ];

	FILE *fp = fopen(filename, "w");
	if (!fp)
		return -1;

	n = SSL_read(hc->ssl, buff, sizeof(buff));
	if (n < 0)
		goto out;

	int body_pos = rtl_http_get_body_pos(buff, n);
	if (fwrite(buff + body_pos, sizeof(uint8_t), n - body_pos, fp) != n - body_pos)
		goto out;

	for (;;) {
		n = SSL_read(hc->ssl, buff, sizeof(buff));
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
	fclose(fp);

	return ret;
}

void rtl_https_close_connection(struct rtl_https_connection *hc)
{
	SSL_shutdown(hc->ssl);
	SSL_free(hc->ssl);
	SSL_CTX_free(hc->ctx);
	rtl_socket_connection_close(hc->sc);
	free(hc);
}
