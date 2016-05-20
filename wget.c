#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "url.h"
#include "http.h"
#include "socket.h"
#include "debug.h"

/*
 * so far, just support http
 */
int wget(const char *orignal_url, const char *filename)
{
	url_field_t *url = url_parse(orignal_url);
	if (url == NULL)
		return -1;

	int sockfd;
	int port = url->port == NULL ? 80 : atoi(url->port);
	if ((sockfd = socket_connect(url->host, port)) < 0) {
		debug("socket_connect error: %s", strerror(errno));
		return -1;
	}

	char buff[BUFSIZ];
	int n = http_build_get_header(url->host, url->path, buff);
	url_free(url);

	if (socket_send(sockfd, buff, n) < 0) {
		debug("socket_send error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	/* store to file */
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		debug("fopen error: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	int ret = 0;
	/* receive http header first */
	n = socket_recv(sockfd, buff, sizeof(buff));
	if (n < 0) {
		debug("socket_recv error: %s", strerror(errno));
		ret = -1;
		goto out;
	}

	int offset = http_get_body_pos(buff, n);
	fwrite(buff + offset, sizeof(char), n - offset, fp);

	for (;;) {
		n = socket_recv(sockfd, buff, sizeof(buff));
		if (n > 0) {
			fwrite(buff, sizeof(char), n, fp);
		} else if (n == 0) {	/* receive done */
			break;
		} else {
			debug("socket_recv error: %s", strerror(errno));
			ret = -1;
			goto out;
		}
	}
out:
	close(sockfd);
	fclose(fp);
	return ret;
}
