#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rtl_http_req.h"

const char *http_req_type_char[] = {
	"GET",
	"OPTIONS",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"CONNECT",
	"PROPFIND",
	"PROPPATCH",
	"MKCOL",
	"COPY",
	"MOVE",
	"LOCK",
	"UNLOCK",
	NULL
};

rtl_http_req_t *rtl_http_req_new(rtl_http_req_type_t type, const char *host, int port,
								 const char *path)
{
	rtl_http_req_t *req;

	if (!host || !path)
		return NULL;

	req = calloc(1, sizeof(rtl_http_req_t));
	if (!req)
		return NULL;

	req->type = type;
	req->host = strdup(host);
	if (!req->host)
		goto err1;
	req->port = port;
	req->path = strdup(path);
	if (!req->path)
		goto err2;
	req->headers = rtl_http_hdr_list_new();
	if (!req->headers)
		goto err3;

	rtl_http_hdr_set_value(req->headers, RTL_HTTP_HDR_Host, req->host);

	return req;

err3:
	free(req->path);
err2:
	free(req->host);
err1:
	free(req);
	return NULL;
}

void rtl_http_req_destroy(rtl_http_req_t *req)
{
	if (!req)
		return;
	if (req->host)
		free(req->host);
	if (req->path)
		free(req->path);
	if (req->headers)
		rtl_http_hdr_list_destroy(req->headers);
	free(req);
}

struct rtl_socket_connection *rtl_http_req_conn(rtl_http_req_t *req)
{
	if (!req)
		return NULL;
	return rtl_socket_tcp_connect(req->host, req->port);
}

void rtl_http_req_discon(struct rtl_socket_connection *conn)
{
	if (!conn)
		return;
	rtl_socket_disconnect(conn);
	conn = NULL;
}

int rtl_http_req_send_hdr(const rtl_http_req_t *req,
						  struct rtl_socket_connection *conn,
						  int content_len)
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
		if (content_len > 0) {
			snprintf(len, sizeof(len), "%d", content_len);
			rtl_http_hdr_set_value(req->headers, RTL_HTTP_HDR_Content_Length, len);
		}
	}

	if (rtl_http_hdr_to_string(hdr, sizeof(hdr), req->headers) < 0)
		return -1;

	if (path[0] == '/')
		path++;

	snprintf(buf, sizeof(buf), tpl, http_req_type_char[req->type], path, hdr);

	return rtl_socket_sendn(conn->fd, buf, strlen(buf));
}

int rtl_http_req_send_body(const rtl_http_req_t *req,
						   struct rtl_socket_connection *conn,
						   const unsigned char *body, int body_len)
{
	if ((req->type == RTL_HTTP_REQ_TYPE_POST) ||
		(req->type == RTL_HTTP_REQ_TYPE_PUT) ||
		(req->type == RTL_HTTP_REQ_TYPE_TRACE)) {
		if (body && body_len > 0)
			return rtl_socket_sendn(conn->fd, body, body_len);
	}

	return -1;
}
