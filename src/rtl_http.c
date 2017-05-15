#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "rtl_http.h"

/*
 * @header: built header
 * @size: size of header
 * @path: the path to get
 * @hostname: the remote hostname
 */
int rtl_http_build_get_header(char *header, size_t size, const char *path,
							  const char *hostname, int keepalive)
{
	const char *getpath = path;
	char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: %s\r\n\r\n";

	if (getpath[0] == '/')
		getpath++;

	if (keepalive)
		snprintf(header, size, tpl, getpath, hostname, "keep-alive");
	else
		snprintf(header, size, tpl, getpath, hostname, "close");

	return strlen(header);
}

/*
 * @header: built header
 * @size: size of header
 * @path: the path to post
 * @hostname: the remote hostname
 * @content_len: content length
 */
int rtl_http_build_post_header(char *header, size_t size, const char *path,
							   const char *hostname, int content_len, int keepalive)
{
	const char *postpath = path;
	char *tpl = "POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n";

	if (postpath[0] == '/')
		postpath++;

	if (keepalive)
		snprintf(header, size, tpl, postpath, hostname, content_len, "keep-alive");
	else
		snprintf(header, size, tpl, postpath, hostname, content_len, "close");

	return strlen(header);
}

/*
 * @buff: stored response packet
 * @size: size of buff
 */
int rtl_http_get_body_pos(const uint8_t *buff, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (strncmp((const char *)buff + i, "\r\n\r\n", 4) == 0) {
			return i + 4;
		}
	}
	return -1;
}

struct rtl_http_connection *rtl_http_send_get_request(const char *path, const char *host, uint16_t port, int keepalive)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_http_connection *hc;

	header_len = rtl_http_build_get_header(header, sizeof(header), path, host, keepalive);

	hc = rtl_socket_tcp_connect(host, port);
	if (!hc)
		return NULL;

	if (rtl_socket_send(hc->fd, header, header_len) < 0) {
		rtl_http_close_connection(hc);
		return NULL;
	}

	return hc;
}

struct rtl_http_connection *rtl_http_send_post_request(const char *path, const char *host,
													   uint16_t port, const uint8_t *body,
													   size_t body_len, int keepalive)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_http_connection *hc;

	header_len = rtl_http_build_post_header(header, sizeof(header), path, host,
											body_len, keepalive);

	hc = rtl_socket_tcp_connect(host, port);
	if (!hc)
		return NULL;

	if (rtl_socket_send(hc->fd, header, header_len) < 0) {
		rtl_http_close_connection(hc);
		return NULL;
	}

	if (rtl_socket_send(hc->fd, body, body_len) < 0) {
		rtl_http_close_connection(hc);
		return NULL;
	}

	return hc;
}

int rtl_http_recv_response(struct rtl_http_connection *hc, uint8_t *resp, size_t size)
{
	return rtl_socket_recv(hc->fd, resp, size);
}

int rtl_http_save_body_to_file(struct rtl_http_connection *hc, const char *filename)
{
	uint8_t buff[BUFSIZ];

	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		return -1;

	int ret = -1;
	int n = rtl_socket_recv(hc->fd, buff, sizeof(buff));
	if (n < 0)
		goto out;

	int body_pos = rtl_http_get_body_pos(buff, n);
	if (fwrite(buff + body_pos, sizeof(uint8_t), n - body_pos, fp) != n - body_pos)
		goto out;

	for (;;) {
		n = rtl_socket_recv(hc->fd, buff, sizeof(buff));
		if (n > 0) {
			if (fwrite(buff, sizeof(uint8_t), n, fp) != n)
				goto out;
		} else if (n == 0) {	/* receive done */
			break;
		} else {
			goto out;
		}
	}

	ret = 0;

out:
	fclose(fp);
	return ret;
}

void rtl_http_close_connection(struct rtl_http_connection *hc)
{
	rtl_socket_connection_close(hc);
}
