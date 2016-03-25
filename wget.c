#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "url.h"
#include "http.h"
#include "socket.h"

#define BUFF_SIZE	4096

int wget(const char *orignal_url, const char *filename)
{
	url_field_t *url = url_parse(orignal_url);
	if (url == NULL)
		return -1;

	int sockfd;
	int port = url->port == NULL ? 80 : atoi(url->port);
	if ((sockfd = socket_connect(url->host, url->host_type, port)) < 0) {
	#ifdef DEBUG
		perror("socket_connect()");
	#endif
		return -1;
	}

	char buff[BUFF_SIZE];
	int n = http_build_get_header(url->host, url->path, buff);
	url_free(url);

	if (socket_send(sockfd, buff, n) < 0) {
	#ifdef DEBUG
		perror("socket_send()");
	#endif
		return -1;
	}

	/* store to file */
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
	#ifdef DEBUG
		perror("fopen()");
	#endif
		return -1;
	}

	/* receive http header first */
	n = socket_recv(sockfd, buff, sizeof(buff));
	if (n < 0) {
	#ifdef DEBUG
		perror("socket_recv()");
	#endif
		return -1;
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
		#ifdef DEBUG
			perror("socket_recv()");
		#endif
			return -1;
		}
	}
	close(sockfd);
	fclose(fp);
	return 0;
}
