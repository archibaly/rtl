#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "rtl_fcgi.h"
#include "rtl_readn.h"
#include "rtl_writen.h"
#include "rtl_hash.h"

#ifndef MAXFQDNLEN
#define MAXFQDNLEN 255
#endif

#define HASH_BUKET_SIZE	111

typedef struct fcgi_header {
	unsigned char version;
	unsigned char type;
	unsigned char requestIdB1;
	unsigned char requestIdB0;
	unsigned char contentLengthB1;
	unsigned char contentLengthB0;
	unsigned char paddingLength;
	unsigned char reserved;
} fcgi_header_t;

typedef struct fcgi_begin_request {
	unsigned char roleB1;
	unsigned char roleB0;
	unsigned char flags;
	unsigned char reserved[5];
} fcgi_begin_request_t;

typedef struct fcgi_begin_request_rec {
	fcgi_header_t hdr;
	fcgi_begin_request_t body;
} fcgi_begin_request_rec_t;

typedef struct fcgi_end_request {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} fcgi_end_request_t;

typedef struct fcgi_end_request_rec {
	fcgi_header_t hdr;
	fcgi_end_request_t body;
} fcgi_end_request_rec_t;

struct rtl_fcgi {
	int listen_sock;
	int conn_sock;
	int id;
	int keep;
	int ended;

	struct rtl_hash_table *env;

	int in_len;
	unsigned char *in_buf;

	fcgi_header_t *out_hdr;
	unsigned char *out_pos;
	unsigned char out_buf[1024*8];
	unsigned char reserved[sizeof(fcgi_end_request_rec_t)];
};

typedef union sa {
	struct sockaddr     sa;
	struct sockaddr_un  sa_unix;
	struct sockaddr_in  sa_inet;
	struct sockaddr_in6 sa_inet6;
} sa_t;

static int fcgi_read_request();

static void fcgi_signal_handler(int signo)
{
	_exit(0);
}

static void fcgi_setup_signals(void)
{
	struct sigaction new_sa, old_sa;

	sigemptyset(&new_sa.sa_mask);
	new_sa.sa_flags = 0;
	new_sa.sa_handler = fcgi_signal_handler;
	sigaction(SIGTERM, &new_sa, NULL);
	sigaction(SIGPIPE, NULL, &old_sa);
	if (old_sa.sa_handler == SIG_DFL)
		sigaction(SIGPIPE, &new_sa, NULL);
}

static int fcgi_listen(const char *path, uint16_t port)
{
	int listen_sock;
	sa_t sa;
	socklen_t sock_len;
	int reuse = 1;

	/* prepare socket address */
	if (port != 0) {
		memset(&sa.sa_inet, 0, sizeof(sa.sa_inet));
		sa.sa_inet.sin_family = AF_INET;
		sa.sa_inet.sin_port = htons(port);
		sock_len = sizeof(sa.sa_inet);

		if (!*path || !strncmp(path, "*", sizeof("*") - 1)) {
			sa.sa_inet.sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			sa.sa_inet.sin_addr.s_addr = inet_addr(path);
			if (sa.sa_inet.sin_addr.s_addr == INADDR_NONE) {
				struct hostent *hep;

				if (strlen(path) > MAXFQDNLEN)
					hep = NULL;
				else
					hep = gethostbyname(path);

				if (!hep || hep->h_addrtype != AF_INET || !hep->h_addr_list[0]) {
					/* log_write(LOG_ERRO, "Cannot resolve host name '%s'!", host); */
					return -1;
				} else if (hep->h_addr_list[1]) {
					/* log_write(LOG_ERRO, "Host '%s' has multiple addresses. You must choose one explicitly!", host); */
					return -1;
				}
				sa.sa_inet.sin_addr.s_addr = ((struct in_addr*)hep->h_addr_list[0])->s_addr;
			}
		}
	} else {
		int path_len = strlen(path);

		if (path_len >= (int)sizeof(sa.sa_unix.sun_path)) {
			/* log_write(LOG_ERRO, "Listening socket's path name is too long."); */
			return -1;
		}

		memset(&sa.sa_unix, 0, sizeof(sa.sa_unix));
		sa.sa_unix.sun_family = AF_UNIX;
		memcpy(sa.sa_unix.sun_path, path, path_len + 1);
		sock_len = (size_t)(((struct sockaddr_un *)0)->sun_path)	+ path_len;
		unlink(path);
	}

	/* create, bind socket and start listen on it */
	if ((listen_sock = socket(sa.sa.sa_family, SOCK_STREAM, 0)) < 0 ||
	    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0 ||
	    bind(listen_sock, (struct sockaddr *) &sa, sock_len) < 0 ||
	    listen(listen_sock, SOMAXCONN) < 0) {
		close(listen_sock);
		/* log_write(LOG_ERRO, "Cannot bind/listen socket - [%d] %s.",errno, strerror(errno)); */
		return -1;
	}

	fcgi_setup_signals();

	return listen_sock;
}

static inline int fcgi_make_header(fcgi_header_t *hdr, rtl_fcgi_request_type_t type, int req_id, int len)
{
	int padding = ((len + 7) & ~7) - len;

	hdr->contentLengthB0 = (unsigned char)(len & 0xff);
	hdr->contentLengthB1 = (unsigned char)((len >> 8) & 0xff);
	hdr->paddingLength = (unsigned char)padding;
	hdr->requestIdB0 = (unsigned char)(req_id & 0xff);
	hdr->requestIdB1 = (unsigned char)((req_id >> 8) & 0xff);
	hdr->reserved = 0;
	hdr->type = type;
	hdr->version = RTL_FCGI_VERSION_1;
	return padding;
}

rtl_fcgi_t *rtl_fcgi_init(const char *path, uint16_t port)
{
	rtl_fcgi_t *fcgi = calloc(1, sizeof(rtl_fcgi_t));
	if (!fcgi)
		return NULL;

	fcgi->listen_sock = fcgi_listen(path, port);
	if (fcgi->listen_sock < 0) {
		free(fcgi);
		return NULL;
	}

	fcgi->env = rtl_hash_init(HASH_BUKET_SIZE, RTL_HASH_KEY_TYPE_STR);
	if (!fcgi->env) {
		close(fcgi->listen_sock);
		free(fcgi);
		return NULL;
	}

	fcgi->conn_sock = -1;
	fcgi->id = -1;

	return fcgi;
}

static void fcgi_close(rtl_fcgi_t *fcgi, int force, int destroy)
{
	if (destroy)
		rtl_hash_free_nodes(fcgi->env);

	char buf[8];
	if ((force || !fcgi->keep) && fcgi->conn_sock >= 0) {
		if (!force) {
			shutdown(fcgi->conn_sock, 1);
			/* read any remaining data, it may be omitted */
			while (recv(fcgi->conn_sock, buf, sizeof(buf), 0) > 0)
				(void)buf;
		}
		close(fcgi->conn_sock);
		fcgi->conn_sock = -1;
	}
}

int rtl_fcgi_accept(rtl_fcgi_t *fcgi)
{
	for (;;) {
		if (fcgi->conn_sock < 0) {
			for (;;) {
				fcgi->conn_sock = accept(fcgi->listen_sock, NULL, NULL);
				if (fcgi->conn_sock < 0)
					return -1;
				if (fcgi->conn_sock < FD_SETSIZE) {
					fd_set set;
					int ret;

					FD_ZERO(&set);
					FD_SET(fcgi->conn_sock, &set);
					ret = select(fcgi->conn_sock + 1, &set, NULL, NULL, NULL);
					if (ret > 0 && FD_ISSET(fcgi->conn_sock, &set))
						break;
					fcgi_close(fcgi, 1, 0);
				}
			}
		}

		if (fcgi_read_request(fcgi) == 0)
			return fcgi->conn_sock;
		else
			fcgi_close(fcgi, 1, 1);
	}
}

static int fcgi_get_params(rtl_fcgi_t *fcgi, unsigned char *p, unsigned char *end)
{
	unsigned int name_len, val_len;

	while (p < end) {
		name_len = *p++;
		if (name_len >= 128) {
			if (p + 3 >= end)
				return -1;
			name_len = ((name_len & 0x7f) << 24);
			name_len |= (*p++ << 16);
			name_len |= (*p++ << 8);
			name_len |= *p++;
		}
		if (p >= end)
			return -1;
		val_len = *p++;
		if (val_len >= 128) {
			if (p + 3 >= end)
				return -1;
			val_len = ((val_len & 0x7f) << 24);
			val_len |= (*p++ << 16);
			val_len |= (*p++ << 8);
			val_len |= *p++;
		}
		if (name_len + val_len > (unsigned int) (end - p)) {
			/* Malformated request */
			return -1;
		}

		char name[name_len + 1];
		char value[val_len + 1];

		memcpy(name, p, name_len);
		name[name_len] = '\0';

		memcpy(value, p + name_len, val_len);
		value[val_len] = '\0';

		rtl_hash_add(fcgi->env, name, strlen(name) + 1, value, strlen(value) + 1);
		p += name_len + val_len;
	}
	return 0;
}

static int fcgi_read_request(rtl_fcgi_t *fcgi)
{
	fcgi_header_t hdr;
	int len, padding;
	unsigned char buf[RTL_FCGI_MAX_LENGTH];

	fcgi->keep = 0;
	fcgi->ended = 0;
	fcgi->in_len = 0;
	fcgi->out_hdr = NULL;
	fcgi->out_pos = fcgi->out_buf;

	/* begin request */
	if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
		hdr.version < RTL_FCGI_VERSION_1) {
		return -1;
	}
	len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
	padding = hdr.paddingLength;
	if (len + padding > RTL_FCGI_MAX_LENGTH)
		return -1;
	fcgi->id = (hdr.requestIdB1 << 8) + hdr.requestIdB0;
	if (hdr.type == RTL_FCGI_BEGIN_REQUEST && len == sizeof(fcgi_begin_request_t)) {
		fcgi_begin_request_t *b;

		if (rtl_readn(fcgi->conn_sock, buf, len + padding) != len + padding)
			return -1;
		b = (fcgi_begin_request_t *)buf;
		fcgi->keep = (b->flags & RTL_FCGI_KEEP_CONN);

		switch ((b->roleB1 << 8) + b->roleB0) {
			case RTL_FCGI_RESPONDER:
				rtl_hash_add(fcgi->env, "FCGI_ROLE", sizeof("FCGI_ROLE"), "RESPONDER", sizeof("RESPONDER"));
				break;
			case RTL_FCGI_AUTHORIZER:
				rtl_hash_add(fcgi->env, "FCGI_ROLE", sizeof("FCGI_ROLE"), "AUTHORIZER", sizeof("AUTHORIZER"));
				break;
			case RTL_FCGI_FILTER:
				rtl_hash_add(fcgi->env, "FCGI_ROLE", sizeof("FCGI_ROLE"), "FILTER", sizeof("FILTER"));
				break;
			default:
				return -1;
		}

		/* params */
		if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
		    hdr.version < RTL_FCGI_VERSION_1) {
			return -1;
		}
		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;
		while (hdr.type == RTL_FCGI_PARAMS && len > 0) {
			if (len + padding > RTL_FCGI_MAX_LENGTH)
				return -1;
			if (rtl_readn(fcgi->conn_sock, buf, len + padding) != len + padding)
				return -1;
			if (fcgi_get_params(fcgi, buf, buf + len) < 0)
				return -1;
			if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
			    hdr.version < RTL_FCGI_VERSION_1) {
				return -1;
			}
			len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			padding = hdr.paddingLength;
		}

		/* stdin */
		if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
		    hdr.version < RTL_FCGI_VERSION_1) {
			return -1;
		}
		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;
		while (hdr.type == RTL_FCGI_STDIN && len > 0) {
			if (len + padding > RTL_FCGI_MAX_LENGTH)
				return -1;

			unsigned char *p = realloc(fcgi->in_buf, fcgi->in_len + len);
			if (!p)
				return -1;
			fcgi->in_buf = p;

			if (rtl_readn(fcgi->conn_sock, fcgi->in_buf + fcgi->in_len, len) != len)
				return -1;
			fcgi->in_len += len;

			if (rtl_readn(fcgi->conn_sock, buf, padding) != padding)
				return -1;

			if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
			    hdr.version < RTL_FCGI_VERSION_1) {
				return -1;
			}
			len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			padding = hdr.paddingLength;
		}
	}

	return 0;
}

/*
int rtl_fcgi_write(rtl_fcgi_t *fcgi, const void *buf, size_t count)
{
	int i, n;
	char tmp[8] = "";
	fcgi_header_t hdr;
	unsigned char out_buf[RTL_FCGI_MAX_LENGTH + 8];
	int out_len;

	n = count / RTL_FCGI_MAX_LENGTH;
	for (i = 0; i < n; i++) {
		fcgi_make_header(&hdr, RTL_FCGI_STDOUT, fcgi->id, RTL_FCGI_MAX_LENGTH);
		if (rtl_writen(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr))
			return -1;
		if (rtl_writen(fcgi->conn_sock, buf + i * RTL_FCGI_MAX_LENGTH, RTL_FCGI_MAX_LENGTH) != RTL_FCGI_MAX_LENGTH)
			return -1;
	}

	int left = count - n * RTL_FCGI_MAX_LENGTH;
	if (left > 0) {
		int padding = fcgi_make_header(&hdr, RTL_FCGI_STDOUT, fcgi->id, left);
		memcpy(out_buf, &hdr, sizeof(hdr));
		memcpy(out_buf + sizeof(hdr), buf + i * RTL_FCGI_MAX_LENGTH, left);
		memcpy(out_buf + sizeof(hdr) + left, tmp, padding);
		out_len = sizeof(hdr) + left + padding;
		if (rtl_writen(fcgi->conn_sock, out_buf, out_len) != out_len)
			return -1;
	}
	return 0;
}
*/

int rtl_fcgi_printf(rtl_fcgi_t *fcgi, const char *fmt, ...)
{
	int ret;
	va_list args;
	char *buf;

	va_start(args, fmt);
	ret = vasprintf(&buf, fmt, args);
	va_end(args);

	if (ret < 0)
		return -1;
	if (rtl_fcgi_write(fcgi, RTL_FCGI_STDOUT, buf, strlen(buf)) < 0) {
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}

static inline fcgi_header_t *open_packet(rtl_fcgi_t *fcgi, rtl_fcgi_request_type_t type)
{
	fcgi->out_hdr = (fcgi_header_t *)fcgi->out_pos;
	fcgi->out_hdr->type = type;
	fcgi->out_pos += sizeof(fcgi_header_t);
	return fcgi->out_hdr;
}

static inline void close_packet(rtl_fcgi_t *fcgi)
{
	int len;
	if (fcgi->out_hdr) {
		len = (int)(fcgi->out_pos - ((unsigned char *)fcgi->out_hdr + sizeof(fcgi_header_t)));
		fcgi->out_pos += fcgi_make_header(fcgi->out_hdr, (rtl_fcgi_request_type_t)fcgi->out_hdr->type, fcgi->id, len);
		fcgi->out_hdr = NULL;
	}
}

static int fcgi_flush(rtl_fcgi_t *fcgi, int end)
{
	int len;

	close_packet(fcgi);

	len = (int)(fcgi->out_pos - fcgi->out_buf);

	if (end) {
		fcgi_end_request_rec_t *rec = (fcgi_end_request_rec_t *)(fcgi->out_pos);

		fcgi_make_header(&rec->hdr, RTL_FCGI_END_REQUEST, fcgi->id, sizeof(fcgi_end_request_t));
		rec->body.appStatusB3 = 0;
		rec->body.appStatusB2 = 0;
		rec->body.appStatusB1 = 0;
		rec->body.appStatusB0 = 0;
		rec->body.protocolStatus = RTL_FCGI_REQUEST_COMPLETE;
		len += sizeof(fcgi_end_request_rec_t);
	}

	if (rtl_writen(fcgi->conn_sock, fcgi->out_buf, len) != len) {
		fcgi->keep = 0;
		fcgi->out_pos = fcgi->out_buf;
		return 0;
	}

	fcgi->out_pos = fcgi->out_buf;
	return 1;
}

int rtl_fcgi_write(rtl_fcgi_t *fcgi, rtl_fcgi_request_type_t type, const char *str, int len)
{
	int limit, rest;

	if (len <= 0)
		return 0;

	if (fcgi->out_hdr && fcgi->out_hdr->type != type)
		close_packet(fcgi);
#if 0
	/* Unoptimized, but clear version */
	rest = len;
	while (rest > 0) {
		limit = sizeof(fcgi->out_buf) - (fcgi->out_pos - fcgi->out_buf);

		if (!fcgi->out_hdr) {
			if (limit < sizeof(fcgi_header)) {
				if (!fcgi_flush(fcgi, 0)) {
					return -1;
				}
			}
			open_packet(fcgi, type);
		}
		limit = sizeof(fcgi->out_buf) - (fcgi->out_pos - fcgi->out_buf);
		if (rest < limit) {
			memcpy(fcgi->out_pos, str, rest);
			fcgi->out_pos += rest;
			return len;
		} else {
			memcpy(fcgi->out_pos, str, limit);
			fcgi->out_pos += limit;
			rest -= limit;
			str += limit;
			if (!fcgi_flush(fcgi, 0)) {
				return -1;
			}
		}
	}
#else
	/* Optimized version */
	limit = (int)(sizeof(fcgi->out_buf) - (fcgi->out_pos - fcgi->out_buf));
	if (!fcgi->out_hdr) {
		limit -= sizeof(fcgi_header_t);
		if (limit < 0)
			limit = 0;
	}

	if (len < limit) {
		if (!fcgi->out_hdr) {
			open_packet(fcgi, type);
		}
		memcpy(fcgi->out_pos, str, len);
		fcgi->out_pos += len;
	} else if (len - limit < (int)(sizeof(fcgi->out_buf) - sizeof(fcgi_header_t))) {
		if (!fcgi->out_hdr) {
			open_packet(fcgi, type);
		}
		if (limit > 0) {
			memcpy(fcgi->out_pos, str, limit);
			fcgi->out_pos += limit;
		}
		if (!fcgi_flush(fcgi, 0)) {
			return -1;
		}
		if (len > limit) {
			open_packet(fcgi, type);
			memcpy(fcgi->out_pos, str + limit, len - limit);
			fcgi->out_pos += len - limit;
		}
	} else {
		int pos = 0;
		int pad;

		close_packet(fcgi);
		while ((len - pos) > 0xffff) {
			open_packet(fcgi, type);
			fcgi_make_header(fcgi->out_hdr, type, fcgi->id, 0xfff8);
			fcgi->out_hdr = NULL;
			if (!fcgi_flush(fcgi, 0)) {
				return -1;
			}
			if (rtl_writen(fcgi->conn_sock, str + pos, 0xfff8) != 0xfff8) {
				fcgi->keep = 0;
				return -1;
			}
			pos += 0xfff8;
		}

		pad = (((len - pos) + 7) & ~7) - (len - pos);
		rest = pad ? 8 - pad : 0;

		open_packet(fcgi, type);
		fcgi_make_header(fcgi->out_hdr, type, fcgi->id, (len - pos) - rest);
		fcgi->out_hdr = NULL;
		if (!fcgi_flush(fcgi, 0)) {
			return -1;
		}
		if (rtl_writen(fcgi->conn_sock, str + pos, (len - pos) - rest) != (len - pos) - rest) {
			fcgi->keep = 0;
			return -1;
		}
		if (pad) {
			open_packet(fcgi, type);
			memcpy(fcgi->out_pos, str + len - rest,  rest);
			fcgi->out_pos += rest;
		}
	}
#endif
	return len;
}

static int fcgi_end(rtl_fcgi_t *fcgi)
{
	int ret = -1;
	if (!fcgi->ended) {
		ret = fcgi_flush(fcgi, 1);
		fcgi->ended = 1;
	}
	return ret;
}

int rtl_fcgi_finish(rtl_fcgi_t *fcgi)
{
	int ret = -1;

	if (fcgi->in_buf) {
		free(fcgi->in_buf);
		fcgi->in_buf = NULL;
		fcgi->in_len = 0;
	}
	if (fcgi->conn_sock >= 0) {
		ret = fcgi_end(fcgi);
		fcgi_close(fcgi, 1, 1);
	}
	return ret;
}

int rtl_fcgi_get_stdin(rtl_fcgi_t *fcgi, unsigned char *buf)
{
	if (!buf)
		return -1;
	buf = fcgi->in_buf;
	return fcgi->in_len;
}

char *rtl_fcgi_getenv(const rtl_fcgi_t *fcgi, const char *name)
{
	void *value;
	if (rtl_hash_find(fcgi->env, name, &value, 1) == 0)
		return NULL;
	return value;
}
