#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "rtl_url.h"
#include "rtl_http.h"
#include "rtl_https.h"

static int wget_http(const char *filename, const char *path, const char *host,
					 int port)
{
	struct rtl_http_connection *hc = rtl_http_send_get_request(path, host, port, 0);
	if (!hc)
		return -1;

	int ret = rtl_http_save_body_to_file(hc, filename);
	rtl_http_close_connection(hc);

	return ret;
}

static int wget_https(const char *filename, const char *path, const char *host,
					  int port)
{
	struct rtl_https_connection *hc = rtl_https_send_get_request(path, host, port, 0);
	if (!hc)
		return -1;

	int ret = rtl_https_save_body_to_file(hc, filename);
	rtl_https_close_connection(hc);
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
		ret = wget_http(filename, url->path, url->host, port);
	} else if (strcmp(url->schema, "https") == 0) {
		port = url->port == NULL ? 443 : atoi(url->port);
		ret = wget_https(filename, url->path, url->host, port);
	} else {
		ret = -1;
	}

	rtl_url_free(url);
	return ret;
}
