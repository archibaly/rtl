#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "rtl_url.h"

static char *str_hosttype[] = {"host ipv4", "host ipv6", "host domain", NULL};
static char hex[] = "0123456789ABCDEF";

int rtl_host_is_ipv4(const char *str)
{
	struct sockaddr_in sa;
	return inet_pton(AF_INET, str, &(sa.sin_addr)) != 0;
}

int rtl_host_is_ipv6(const char *str)
{
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, str, &(sa.sin6_addr)) != 0;
}

static void parse_query(rtl_url_field_t *url, char *query)
{
	char *chr;

	chr = strchr(query, '=');
	while (chr) {
		if (url->query)
			url->query =
				realloc(url->query, (url->query_num + 1) * sizeof(*url->query));
		else
			url->query = malloc(sizeof(*url->query));
		url->query[url->query_num].name = strndup(query, chr - query);
		query = chr + 1;
		chr = strchr(query, '&');
		if (chr) {
			url->query[url->query_num].value = strndup(query, chr - query);
			url->query_num++;
			query = chr + 1;
			chr = strchr(query, '=');
		} else {
			url->query[url->query_num].value = strndup(query, -1);
			url->query_num++;
			break;
		}
	}
}

rtl_url_field_t *rtl_url_parse(const char *str)
{
	const char *pch;
	char *query;
	rtl_url_field_t *url;

	query = NULL;
	if ((url = (rtl_url_field_t *)malloc(sizeof(rtl_url_field_t))) == NULL)
		return NULL;

	memset(url, 0, sizeof(rtl_url_field_t));
	if (str && str[0]) {
		url->href = strndup(str, -1);
		pch = strchr(str, ':');	/* parse schema */
		if (pch && pch[1] == '/' && pch[2] == '/') {
			url->schema = strndup(str, pch - str);
			str = pch + 3;
		} else
			goto __fail;
		pch = strchr(str, '@');	/* parse user info */
		if (pch) {
			pch = strchr(str, ':');
			if (pch) {
				url->username = strndup(str, pch - str);
				str = pch + 1;
				pch = strchr(str, '@');
				if (pch) {
					url->password = strndup(str, pch - str);
					str = pch + 1;
				} else
					goto __fail;
			} else
				goto __fail;
		}
		if (str[0] == '[') {	/* parse host info */
			str++;
			pch = strchr(str, ']');
			if (pch) {
				url->host = strndup(str, pch - str);
				str = pch + 1;
				if (str[0] == ':') {
					str++;
					pch = strchr(str, '/');
					if (pch) {
						url->port = strndup(str, pch - str);
						str = pch + 1;
					} else {
						url->port = strndup(str, -1);
						str = str + strlen(str);
					}
				}
				url->host_type = RTL_HOST_IPV6;
			} else
				goto __fail;
		} else {
			const char *pch_slash;

			pch = strchr(str, ':');
			pch_slash = strchr(str, '/');
			if (pch && (!pch_slash || (pch_slash && pch < pch_slash))) {
				url->host = strndup(str, pch - str);
				str = pch + 1;
				pch = strchr(str, '/');
				if (pch) {
					url->port = strndup(str, pch - str);
					str = pch + 1;
				} else {
					url->port = strndup(str, -1);
					str = str + strlen(str);
				}
			} else {
				pch = strchr(str, '/');
				if (pch) {
					url->host = strndup(str, pch - str);
					str = pch + 1;
				} else {
					url->host = strndup(str, -1);
					str = str + strlen(str);
				}
			}
			url->host_type = rtl_host_is_ipv4(url->host) ? RTL_HOST_IPV4 : RTL_HOST_DOMAIN;
		}
		if (str[0]) {			/* parse path, query and fragment */
			pch = strchr(str, '?');
			if (pch) {
				url->path = strndup(str, pch - str);
				str = pch + 1;
				pch = strchr(str, '#');
				if (pch) {
					query = strndup(str, pch - str);
					str = pch + 1;
					url->fragment = strndup(str, -1);
				} else {
					query = strndup(str, -1);
					str = str + strlen(str);
				}
				parse_query(url, query);
				free(query);
			} else {
				pch = strchr(str, '#');
				if (pch) {
					url->path = strndup(str, pch - str);
					str = pch + 1;
					url->fragment = strndup(str, -1);
					str = str + strlen(str);
				} else {
					url->path = strndup(str, -1);
					str = str + strlen(str);
				}
			}
		}
	} else {
__fail:
		rtl_url_free(url);
		return NULL;
	}
	return url;
}

#define ISQSCHR(x)	((((x)=='=')||((x)=='#')||((x)=='&')||((x)=='\0')) ? 0 : 1)

static int hex2dec(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	else if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	else if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	else
		return 0;
}

static int query_strncmp(const char *s, const char *qs, register size_t n)
{
	int i = 0;
	register unsigned char u1, u2, unyb, lnyb;

	while (n-- > 0) {
		u1 = (unsigned char)*s++;
		u2 = (unsigned char)*qs++;

		if (!ISQSCHR(u1)) {
			u1 = '\0';
		}
		if (!ISQSCHR(u2)) {
			u2 = '\0';
		}

		if (u1 == '+') {
			u1 = ' ';
		}
		if (u1 == '%') {		/* easier/safer than scanf */
			unyb = (unsigned char)*s++;
			lnyb = (unsigned char)*s++;
			if (isxdigit(unyb) && isxdigit(lnyb))
				u1 = (hex2dec(unyb) * 16) + hex2dec(lnyb);
			else
				u1 = '\0';
		}

		if (u2 == '+') {
			u2 = ' ';
		}
		if (u2 == '%') {		/* easier/safer than scanf */
			unyb = (unsigned char)*qs++;
			lnyb = (unsigned char)*qs++;
			if (isxdigit(unyb) && isxdigit(lnyb))
				u2 = (hex2dec(unyb) * 16) + hex2dec(lnyb);
			else
				u2 = '\0';
		}

		if (u1 != u2)
			return u1 - u2;
		if (u1 == '\0')
			return 0;
		i++;
	}
	if (ISQSCHR(*qs))
		return -1;
	else
		return 0;
}

static int query_decode(char *qs)
{
	int i = 0, j = 0;

	while (ISQSCHR(qs[j])) {
		if (qs[j] == '+') {
			qs[i] = ' ';
		} else if (qs[j] == '%') {	/* easier/safer than scanf */
			if (!isxdigit(qs[j + 1]) || !isxdigit(qs[j + 2])) {
				qs[i] = '\0';
				return i;
			}
			qs[i] = (hex2dec(qs[j + 1]) * 16) + hex2dec(qs[j + 2]);
			j += 2;
		} else {
			qs[i] = qs[j];
		}
		i++;
		j++;
	}
	qs[i] = '\0';

	return i;
}

int rtl_url_query_parse(char *qs, char **qs_kv, int qs_kv_size)
{
	int i, j;
	char *substr_ptr;

	for (i = 0; i < qs_kv_size; i++)
		qs_kv[i] = NULL;

	/* find the beginning of the k/v substrings */
	if ((substr_ptr = strchr(qs, '?')) != NULL)
		substr_ptr++;
	else
		substr_ptr = qs;

	i = 0;
	while (i < qs_kv_size) {
		qs_kv[i] = substr_ptr;
		j = strcspn(substr_ptr, "&");
		if (substr_ptr[j] == '\0') {
			break;
		}
		substr_ptr += j + 1;
		i++;
	}
	i++;						/* x &'s -> means x iterations of this loop -> means *x+1* k/v pairs */

	/* we only decode the values in place, the keys could have '='s in them */
	/* which will hose our ability to distinguish keys from values later */
	for (j = 0; j < i; j++) {
		substr_ptr = qs_kv[j] + strcspn(qs_kv[j], "=&#");
		if (substr_ptr[0] == '&')	/* blank value: skip decoding */
			substr_ptr[0] = '\0';
		else
			query_decode(++substr_ptr);
	}

	return i;
}

char *rtl_url_query_k2v(const char *key, char **qs_kv, int qs_kv_size)
{
	int i;
	size_t key_len, skip;

	key_len = strlen(key);

	for (i = 0; i < qs_kv_size; i++) {
		/* we rely on the unambiguous '=' to find the value in our k/v pair */
		if (query_strncmp(key, qs_kv[i], key_len) == 0) {
			skip = strcspn(qs_kv[i], "=");
			if (qs_kv[i][skip] == '=')
				skip++;
			/* return (zero-char value) ? ptr to trailing '\0' : ptr to value */
			return qs_kv[i] + skip;
		}
	}

	return NULL;
}

char *rtl_url_query_scanvalue(const char *key, const char *qs, char *val, size_t val_len)
{
	int i, key_len;
	char *tmp;

	/* find the beginning of the k/v substrings */
	if ((tmp = strchr(qs, '?')) != NULL)
		qs = tmp + 1;

	key_len = strlen(key);
	while (qs[0] != '#' && qs[0] != '\0') {
		if (query_strncmp(key, qs, key_len) == 0)
			break;
		qs += strcspn(qs, "&") + 1;
	}

	if (qs[0] == '\0')
		return NULL;

	qs += strcspn(qs, "=&#");
	if (qs[0] == '=') {
		qs++;
		i = strcspn(qs, "&=#");
		strncpy(val, qs, (val_len - 1) < (i + 1) ? (val_len - 1) : (i + 1));
		query_decode(val);
	} else {
		if (val_len > 0)
			val[0] = '\0';
	}

	return val;
}

void rtl_url_free(rtl_url_field_t *url)
{
	if (!url)
		return;

	free(url->href);	/* free NULL, that's ok */
	free(url->schema);
	free(url->username);
	free(url->password);
	free(url->host);
	free(url->port);
	free(url->path);

	int i;
	for (i = 0; i < url->query_num; i++) {
		free(url->query[i].name);
		free(url->query[i].value);
	}
	free(url->query);
	free(url->fragment);
	free(url);
}

void rtl_url_field_print(rtl_url_field_t *url)
{
	if (!url)
		return;

	fprintf(stdout, "\nurl field:\n");
	fprintf(stdout, "  - href:     '%s'\n", url->href);
	fprintf(stdout, "  - schema:   '%s'\n", url->schema);
	if (url->username)
		fprintf(stdout, "  - username: '%s'\n", url->username);
	if (url->password)
		fprintf(stdout, "  - password: '%s'\n", url->password);
	fprintf(stdout, "  - host:     '%s' (%s)\n", url->host,
			str_hosttype[url->host_type]);
	if (url->port)
		fprintf(stdout, "  - port:     '%s'\n", url->port);
	if (url->path)
		fprintf(stdout, "  - path:     '%s'\n", url->path);
	if (url->query_num > 0) {
		int i;

		fprintf(stdout, "  - query\n");
		for (i = 0; i < url->query_num; i++) {
			fprintf(stdout, "    * %s : %s\n", url->query[i].name,
					url->query[i].value);
		}
	}
	if (url->fragment)
		fprintf(stdout, "  - fragment: '%s'\n", url->fragment);
}

char *rtl_url_get_file_name(const char *url)
{
	const char *p = url;
	const char *find, *tmp = NULL;

	for (find = p; find; tmp = p, p = find, p++)
		find = strstr(p, "/");

	return (char *)tmp;
}

static char from_hex(char ch)
{
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

static char to_hex(char code)
{
	return hex[code & 15];
}

/* IMPORTANT: be sure to free() the returned string after use */
char *rtl_url_encode(char *str)
{
	char *pstr = str;
	char *buf = malloc(strlen(str) * 3 + 1);
	char *pbuf = buf;

	if (!pbuf)
		return NULL;

	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' ||
			*pstr == '~') {
			*pbuf++ = *pstr;
		} else if (*pstr == ' ') {
			*pbuf++ = '+';
		} else {
			*pbuf++ = '%';
			*pbuf++ = to_hex(*pstr >> 4);
			*pbuf++ = to_hex(*pstr & 15);
		}
		pstr++;
	}
	*pbuf = '\0';

	return buf;
}

/* IMPORTANT: be sure to free() the returned string after use */
char *rtl_url_decode(char *str)
{
	char *pstr = str;
	char *buf = malloc(strlen(str) + 1);
	char *pbuf = buf;

	if (!pbuf)
		return NULL;

	while (*pstr) {
		if (*pstr == '%') {
			if (pstr[1] && pstr[2]) {
				*pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
				pstr += 2;
			}
		} else if (*pstr == '+') {
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *pstr;
		}
		pstr++;
	}
	*pbuf = '\0';

	return buf;
}
