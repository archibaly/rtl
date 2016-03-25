#include <stdio.h>
#include <string.h>

#include "http.h"

/**
 * http_build_get_header - build http GET header
 * @hostname: in
 * @path: in
 * @header: out
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

/**
 * http_build_post_header - build http POST header
 * @hostname: in
 * @path: in
 * @header: out
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

/**
 * http_get_body_pos - get http body position
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
