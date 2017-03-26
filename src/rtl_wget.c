#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "rtl_url.h"
#include "rtl_http.h"
#include "rtl_https.h"
#include "rtl_socket.h"
#include "rtl_debug.h"

static int wget_http(const char *filename, const char *path, const char *host,
					 int port)
{
	struct rtl_socket_connection *sc = rtl_http_send_get_request(path, host, port);
	if (!sc)
		return -1;

	return rtl_http_save_body_to_file(sc, filename);
}

static int wget_https(const char *filename, const char *path, const char *host,
					  int port)
{
	struct ssl ssl;

	if (rtl_https_send_get_request(&ssl, path, host, port) < 0)
		return -1;

	return rtl_https_save_body_to_file(&ssl, filename);
}

/* so far, just support http and https */
int rtl_wget(const char *orignal_url)
{
	char *filename = rtl_get_file_name_from_url(orignal_url);
	if (!filename)
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
