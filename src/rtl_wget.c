#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "rtl_url.h"
#include "rtl_http.h"
#include "rtl_socket.h"
#include "rtl_debug.h"

/* so far, just support http */
int rtl_wget(const char *orignal_url, const char *filename)
{
	rtl_url_field_t *url = rtl_url_parse(orignal_url);
	if (url == NULL)
		return -1;

	int sockfd;
	int port = url->port == NULL ? 80 : atoi(url->port);
	if ((sockfd = rtl_socket_connect(url->host, port)) < 0) {
		rtl_debug("rtl_socket_connect error: %s", strerror(errno));
		return -1;
	}

	char buff[BUFSIZ];
	int n = rtl_http_build_get_header(url->host, url->path, buff);
	rtl_url_free(url);

	if (rtl_socket_send(sockfd, buff, n) < 0) {
		rtl_debug("rtl_socket_send error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	/* store to file */
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		rtl_debug("fopen error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	int ret = 0;
	/* receive http header first */
	n = rtl_socket_recv(sockfd, buff, sizeof(buff));
	if (n < 0) {
		rtl_debug("rtl_socket_recv error: %s", strerror(errno));
		ret = -1;
		goto out;
	}

	int offset = rtl_http_get_body_pos(buff, n);
	fwrite(buff + offset, sizeof(char), n - offset, fp);

	for (;;) {
		n = rtl_socket_recv(sockfd, buff, sizeof(buff));
		if (n > 0) {
			fwrite(buff, sizeof(char), n, fp);
		} else if (n == 0) {	/* receive done */
			break;
		} else {
			rtl_debug("rtl_socket_recv error: %s", strerror(errno));
			ret = -1;
			goto out;
		}
	}

out:
	close(sockfd);
	fclose(fp);
	return ret;
}
