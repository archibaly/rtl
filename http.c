#include <stdio.h>
#include <string.h>

#include "http.h"

/*
 * @hostname: the remote hostname
 * @path: the path to get
 * @header: built header
 */
int http_build_get_header(const char *hostname, const char *path, char *header)
{
	const char *getpath = path;
	char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";

	if (getpath[0] == '/')
		getpath++;

	sprintf(header, tpl, getpath, hostname);

	return strlen(header);
}

/*
 * @hostname: the remote hostname
 * @path: the path to post
 * @header: built header
 */
int http_build_post_header(const char *hostname, const char *path, char *header)
{
	const char *postpath = path;
	char *tpl = "POST /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";

	if (postpath[0] == '/')
		postpath++;

	sprintf(header, tpl, postpath, hostname);

	return strlen(header);
}

/*
 * @buff: stored response packet
 * @size: size of buff
 */
int http_get_body_pos(const char *buff, int size)
{
	int i;	
	for (i = 0; i < size; i++) {
		if (strncmp(buff + i, "\r\n\r\n", strlen("\r\n\r\n")) == 0) {
			return i + strlen("\r\n\r\n");
		}
	}
	return -1;
}
