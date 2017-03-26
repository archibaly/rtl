#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "rtl_http.h"
#include "rtl_debug.h"

/*
 * @header: built header
 * @size: size of header
 * @path: the path to get
 * @hostname: the remote hostname
 */
int rtl_http_build_get_header(char *header, size_t size, const char *path,
							  const char *hostname)
{
	const char *getpath = path;
	char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";

	if (getpath[0] == '/')
		getpath++;

	snprintf(header, size, tpl, getpath, hostname);

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
							   const char *hostname, int content_len)
{
	const char *postpath = path;
	char *tpl = "POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n";

	if (postpath[0] == '/')
		postpath++;

	snprintf(header, size, tpl, postpath, hostname, content_len);

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

struct rtl_socket_connection *rtl_http_send_get_request(const char *path, const char *host, uint16_t port)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_socket_connection *sc;

	header_len = rtl_http_build_get_header(header, sizeof(header), path, host);

	sc = rtl_socket_tcp_connect(host, port);
	if (!sc) {
		rtl_debug("rtl_socket_tcp_connect error: %s", strerror(errno));
		return NULL;
	}

	if (rtl_socket_send(sc->fd, header, header_len) < 0) {
		rtl_socket_connection_close(sc);
		return NULL;
	}

	return sc;
}

struct rtl_socket_connection *rtl_http_send_post_request(const char *path, const char *host,
														 uint16_t port, const uint8_t *body, size_t body_len)
{
	char header[BUFSIZ];
	int header_len;
	struct rtl_socket_connection *sc;

	header_len = rtl_http_build_post_header(header, sizeof(header), path, host,
											body_len);

	sc = rtl_socket_tcp_connect(host, port);
	if (!sc)
		return NULL;

	if (rtl_socket_send(sc->fd, header, header_len) < 0) {
		rtl_socket_connection_close(sc);
		return NULL;
	}

	if (rtl_socket_send(sc->fd, body, body_len) < 0) {
		rtl_socket_connection_close(sc);
		return NULL;
	}

	return sc;
}

int rtl_http_recv_response(struct rtl_socket_connection *sc, uint8_t *resp, size_t size)
{
	int n;

	n = rtl_socket_recv(sc->fd, resp, size);
	rtl_socket_connection_close(sc);

	return n;
}

int rtl_http_save_body_to_file(struct rtl_socket_connection *sc, const char *filename)
{
	uint8_t buff[BUFSIZ];

	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		rtl_socket_connection_close(sc);
		return -1;
	}

	int ret = -1;
	int n = recv(sc->fd, buff, sizeof(buff), 0);
	if (n < 0)
		goto out;

	int body_pos = rtl_http_get_body_pos(buff, n);
	if (fwrite(buff + body_pos, sizeof(uint8_t), n - body_pos, fp) != n - body_pos)
		goto out;

	for (;;) {
		n = recv(sc->fd, buff, sizeof(buff), 0);
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
	rtl_socket_connection_close(sc);
	return ret;
}
