#include "rtl_https_req.h"

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

struct rtl_https_connection *rtl_https_req_conn(rtl_http_req_t *req)
{
	struct rtl_https_connection *conn;

	if (!(conn = malloc(sizeof(struct rtl_https_connection))))
		return NULL;

	SSL_library_init();
	conn->ctx = SSL_CTX_new(SSLv23_client_method());
	if (conn->ctx == NULL)
		goto free_conn;

	if (!(conn->ssl = SSL_new(conn->ctx)))
		goto free_ssl_ctx;

	conn->sc = rtl_socket_tcp_connect(req->host, req->port);
	if (!conn->sc)
		goto free_ssl;

	if (SSL_set_fd(conn->ssl, conn->sc->fd) == 0)
		goto free_sockfd;

	if (SSL_connect(conn->ssl) != 1)
		goto free_all;

	return conn;

free_all:
	SSL_shutdown(conn->ssl);
free_sockfd:
	rtl_socket_disconnect(conn->sc);
free_ssl:
	SSL_free(conn->ssl);
free_ssl_ctx:
	SSL_CTX_free(conn->ctx);
free_conn:
	free(conn);

	return NULL;
}

void rtl_https_req_discon(struct rtl_https_connection *conn)
{
	if (!conn)
		return;
	SSL_shutdown(conn->ssl);
	SSL_free(conn->ssl);
	SSL_CTX_free(conn->ctx);
	rtl_socket_disconnect(conn->sc);
	free(conn);
}

int rtl_https_req_send(const rtl_http_req_t *req,
					   struct rtl_https_connection *conn,
					   const unsigned char *body, int body_len)
{
	char len[12];
	char hdr[4096];
	char buf[4096];
	char *path;
	char *tpl = "%s /%s HTTP/1.1\r\n%s\r\n";

	if (!req || !conn)
		return -1;

	if (req->path)
		path = req->path;
	else
		path = "";

	/* check to see if we have an entity body */
	if ((req->type == RTL_HTTP_REQ_TYPE_POST) ||
		(req->type == RTL_HTTP_REQ_TYPE_PUT) ||
		(req->type == RTL_HTTP_REQ_TYPE_TRACE)) {
		if (body && body_len > 0) {
			snprintf(len, sizeof(len), "%d", body_len);
			rtl_http_hdr_set_value(req->headers, RTL_HTTP_HDR_Content_Length, len);
		}
	}

	if (rtl_http_hdr_to_string(hdr, sizeof(hdr), req->headers) < 0)
		return -1;

	if (path[0] == '/')
		path++;

	snprintf(buf, sizeof(buf), tpl, http_req_type_char[req->type], path, hdr);

	if (ssl_writen(conn->ssl, buf, strlen(buf)) < 0)
		return -1;

	if ((req->type == RTL_HTTP_REQ_TYPE_POST) ||
		(req->type == RTL_HTTP_REQ_TYPE_PUT) ||
		(req->type == RTL_HTTP_REQ_TYPE_TRACE)) {
		if (body && body_len > 0) {
			if (ssl_writen(conn->ssl, body, body_len) < 0)
				return -1;
		}
	}

	return 0;
}
