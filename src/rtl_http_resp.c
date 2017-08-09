#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtl_http_resp.h"

rtl_http_resp_t *rtl_http_resp_new(void)
{
	rtl_http_resp_t *resp;

	resp = calloc(1, sizeof(rtl_http_resp_t));
	if (!resp)
		return NULL;
	resp->headers = rtl_http_hdr_list_new();

	return resp;
}

void rtl_http_resp_destroy(rtl_http_resp_t *resp)
{
	if (!resp)
		return;
	if (resp->reason_phrase)
		free(resp->reason_phrase);
	if (resp->headers)
		rtl_http_hdr_list_destroy(resp->headers);
	free(resp);
}

static inline int http_read_byte(rtl_http_resp_t *resp,
								 struct rtl_socket_connection *conn,
								 unsigned char *cp)
{
	if (resp->buf_remain == 0) {
		resp->buf_remain = rtl_socket_recv(conn->fd, resp->buf,
										   sizeof(resp->buf));
		if (resp->buf_remain < 0)
			return -1;
		else if (resp->buf_remain == 0)
			return 0;
		else
			resp->buf_ptr = resp->buf;
	}
	*cp = *resp->buf_ptr++;
	resp->buf_remain--;
	return 1;
}

static int http_read_line(rtl_http_resp_t *resp,
						  struct rtl_socket_connection *conn,
						  char *buf, size_t size)
{
	unsigned char cur;
	char *dst = buf;
	int count = 0, ret;

	while (count < size) {
		ret = http_read_byte(resp, conn, &cur);
		if (ret < 0) {
			return -1;
		} else if (ret == 0) {
			goto out;
		} else {
			if (cur == '\n') {
				*dst = 0;
				return 0;
			}
			if (cur == '\r') {
				continue;
			} else {
				*dst++ = cur;
				count++;
			}
		}
	}
out:
	*dst = 0;
	return 0;
}

static int http_read_buf(rtl_http_resp_t *resp,
						 struct rtl_socket_connection *conn,
						 unsigned char *buf, size_t size)
{
	unsigned char cur;
	unsigned char *dst = buf;
	int count = 0, ret;

	while (count < size) {
		ret = http_read_byte(resp, conn, &cur);
		if (ret < 0)
			return -1;
		else if (ret == 0)
			break;
		*dst++ = cur;
		count++;
	}
	return count;
}

int rtl_http_resp_read_hdrs(rtl_http_resp_t *resp,
							struct rtl_socket_connection *conn)
{
	char line[512];
	int count = 0;
	char *cp, *cp2;
	int ret;

	for (;;) {
		ret = http_read_line(resp, conn, line, sizeof(line));
		if (ret < 0)
			return -1;
		if (line[0] == '\0')
			break;
		count++;
		/*
		 * Scan the status code and reason phrase
		 * example: HTTP/1.1 200 OK
		 */
		if (count == 1) {
			cp = cp2 = line;
			while (*cp2 != ' ')
				cp2++;
			while (*cp2 == ' ')
				cp2++;
			cp = cp2;
			while (*cp2 != ' ')
				cp2++;
			*cp2 = 0;
			resp->status_code = atoi(cp);
			cp = cp2 + 1;
			while (*cp == ' ')
				cp++;
			cp2 = cp;
			while (*cp2 != ' ' && *cp2 != 0)
				cp2++;
			*cp2 = 0;
			resp->reason_phrase = strdup(cp);
			if (!resp->reason_phrase)
				return -1;
			continue;
		}

		/* example: Connection: close */
		cp = cp2 = line;
		while (*cp2 != ':')
			cp2++;
		*cp2 = 0;
		cp2++;
		while (*cp2 == ' ')
			cp2++;
		rtl_http_hdr_set_value(resp->headers, cp, cp2);
	}
	return 0;
}

int rtl_http_resp_read_body(rtl_http_resp_t *resp,
							struct rtl_socket_connection *conn,
							unsigned char *buf, size_t size)
{
	return http_read_buf(resp, conn, buf, size);
}

int rtl_http_resp_get_status_code(rtl_http_resp_t *resp)
{
	if (!resp)
		return -1;
	return resp->status_code;
}

char *rtl_http_resp_get_reason_phrase(rtl_http_resp_t *resp)
{
	if (!resp)
		return NULL;
	return resp->reason_phrase;
}

rtl_http_hdr_list_t *rtl_http_resp_get_hdrs(rtl_http_resp_t *resp)
{
	if (!resp)
		return NULL;
	return resp->headers;
}

int rtl_http_resp_save_body_to_file(rtl_http_resp_t *resp,
									struct rtl_socket_connection *conn,
									const char *filename)
{
	unsigned char buf[BUFSIZ];
	int ret = -1, n;

	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		return -1;

	for (;;) {
		n = rtl_http_resp_read_body(resp, conn, buf, sizeof(buf));
		if (n > 0) {
			if (fwrite(buf, sizeof(unsigned char), n, fp) != n)
				goto out;
		} else if (n == 0) {    /* receive done */
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
