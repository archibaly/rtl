#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "rtl_url.h"
#include "rtl_http_req.h"
#include "rtl_http_resp.h"
#include "rtl_https_req.h"
#include "rtl_https_resp.h"

static int wget_http(const char *filename, const char *host,
					 int port, const char *path)
{
	rtl_http_req_t *req;
	struct rtl_socket_connection *conn;
	rtl_http_resp_t *resp;
	int ret = -1;

	req = rtl_http_req_new(RTL_HTTP_REQ_TYPE_GET, host, port, path);
	if (!req)
		return -1;

	rtl_http_hdr_set_value(req->headers, RTL_HTTP_HDR_Connection, "close");

	if (!(conn = rtl_http_req_conn(req)))
		goto out1;

	if (rtl_http_req_send(req, conn, NULL, 0) < 0)
		goto out2;

	resp = rtl_http_resp_new();
	if (!resp)
		goto out2;

	if (rtl_http_resp_read_hdrs(resp, conn) < 0)
		goto out3;

	ret = rtl_http_resp_save_body_to_file(resp, conn, filename);

out3:
	rtl_http_resp_destroy(resp);
out2:
	rtl_http_req_discon(conn);
out1:
	rtl_http_req_destroy(req);
	return ret;
}

static int wget_https(const char *filename, const char *host,
					  int port, const char *path)
{
	rtl_http_req_t *req;
	struct rtl_https_connection *conn;
	rtl_http_resp_t *resp;
	int ret = -1;

	req = rtl_http_req_new(RTL_HTTP_REQ_TYPE_GET, host, port, path);
	if (!req)
		return -1;

	rtl_http_hdr_set_value(req->headers, RTL_HTTP_HDR_Connection, "close");

	if (!(conn = rtl_https_req_conn(req)))
		goto out1;

	if (rtl_https_req_send(req, conn, NULL, 0) < 0)
		goto out2;

	resp = rtl_http_resp_new();
	if (!resp)
		goto out2;

	if (rtl_https_resp_read_hdrs(resp, conn) < 0)
		goto out3;

	ret = rtl_https_resp_save_body_to_file(resp, conn, filename);

out3:
	rtl_http_resp_destroy(resp);
out2:
	rtl_https_req_discon(conn);
out1:
	rtl_http_req_destroy(req);
	return ret;
}

/* so far, just support http and https */
int rtl_wget(const char *orignal_url, const char *filename)
{
	if (!filename || !orignal_url)
		return -1;

	rtl_url_field_t *url = rtl_url_parse(orignal_url);
	if (url == NULL)
		return -1;

	int ret = 0;
	int port = 0;

	if (strcmp(url->schema, "http") == 0) {
		port = url->port == NULL ? 80 : atoi(url->port);
		ret = wget_http(filename, url->host, port, url->path);
	} else if (strcmp(url->schema, "https") == 0) {
		port = url->port == NULL ? 443 : atoi(url->port);
		ret = wget_https(filename, url->host, port, url->path);
	} else {
		ret = -1;
	}

	rtl_url_free(url);
	return ret;
}
