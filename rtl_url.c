#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtl_url.h"

static char *str_hosttype[] = {"host ipv4", "host ipv6", "host domain", NULL};

extern char *strndup(const char *__string, size_t __n);

int rtl_host_is_ipv4(const char *str)
{
	if (!str)
		return 0;

	while (*str) {
		if ((*str >= '0' && *str <= '9') || *str == '.')
			str++;
		else
			return 0;
	}

	return 1;
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
