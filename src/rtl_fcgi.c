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

typedef union sa {
	struct sockaddr     sa;
	struct sockaddr_un  sa_unix;
	struct sockaddr_in  sa_inet;
	struct sockaddr_in6 sa_inet6;
} sa_t;

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

	return fcgi;
}

int rtl_fcgi_accept(rtl_fcgi_t *fcgi)
{
	int conn_sock;
	if ((conn_sock = accept(fcgi->listen_sock, NULL, NULL)) < 0)
		return -1;
	fcgi->conn_sock = conn_sock;
	return conn_sock;
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

int rtl_fcgi_read_request(rtl_fcgi_t *fcgi)
{
	fcgi_header_t hdr;
	int len, padding;
	unsigned char buf[RTL_FCGI_MAX_LENGTH];

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
#if 0
		fcgi->keep = (b->flags & FCGI_KEEP_CONN);
		if (fcgi->keep && req->tcp && !req->nodelay) {
			int on = 1;

			setsockopt(req->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
			req->nodelay = 1;
		}
#endif
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
			if (rtl_readn(fcgi->conn_sock, buf, len + padding) != len + padding) {
				/* req->keep = 0; */
				return -1;
			}
			if (fcgi_get_params(fcgi, buf, buf + len) < 0) {
				/* req->keep = 0; */
				return -1;
			}
			if (rtl_readn(fcgi->conn_sock, &hdr, sizeof(hdr)) != sizeof(hdr) ||
			    hdr.version < RTL_FCGI_VERSION_1) {
				/* req->keep = 0; */
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

			fcgi->in_buf = realloc(fcgi->in_buf, len);
			if (!fcgi->in_buf)
				return -1;

			if (rtl_readn(fcgi->conn_sock, fcgi->in_buf, len) != len)
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
	if (rtl_fcgi_write(fcgi, buf, strlen(buf)) < 0) {
		free(buf);
		return -1;
	}
	return 0;
}

static int fcgi_end(rtl_fcgi_t *fcgi)
{
	fcgi_end_request_rec_t rec;
	fcgi_make_header(&rec.hdr, RTL_FCGI_END_REQUEST, fcgi->id, sizeof(rec.body));
	rec.body.appStatusB3 = 0;
	rec.body.appStatusB2 = 0;
	rec.body.appStatusB1 = 0;
	rec.body.appStatusB0 = 0;
	rec.body.protocolStatus = RTL_FCGI_REQUEST_COMPLETE;
	if (rtl_writen(fcgi->conn_sock, &rec, sizeof(rec)) != sizeof(rec))
		return -1;
	return 0;
}

unsigned char *rtl_fcgi_get_stdin(rtl_fcgi_t *fcgi, int *len)
{
	if (!len)
		return NULL;
	*len = fcgi->in_len;
	return fcgi->in_buf;
}

char *rtl_fcgi_getenv(const rtl_fcgi_t *fcgi, const char *name)
{
	void *value;
	if (rtl_hash_find(fcgi->env, name, &value, 1) == 0)
		return NULL;
	return value;
}

void rtl_fcgi_finish(rtl_fcgi_t *fcgi)
{
	if (fcgi->in_buf) {
		free(fcgi->in_buf);
		fcgi->in_buf = NULL;
		fcgi->in_len = 0;
	}
	fcgi_end(fcgi);
	close(fcgi->conn_sock);
	rtl_hash_free_nodes(fcgi->env);
}
