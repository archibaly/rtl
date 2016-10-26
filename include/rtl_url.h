#ifndef _RTL_URL_H_
#define _RTL_URL_H_

/*
 * parse url like this
 *
 * schema://username:password@host:port/path?key=value#fragment
 * \____/   \______/ \______/ \__/ \__/ \__/ \_______/ \______/
 *   |         |        |       |    |    |      |         |
 * schema      |     password   |   port  |    query    fragment
 *          username          host      path
 *
 * note:
 *   - username, password, port, path, query, fragment is optional.
 *   - scheme, host must be setting.
 *   - username and password must be paired.
 *
 */

#define RTL_HOST_IPV4	1
#define RTL_HOST_IPV6	2
#define RTL_HOST_DOMAIN	3

typedef struct {
	int host_type;
	char *href;
	char *schema;
	char *username;
	char *password;
	char *host;
	char *port;
	char *path;
	int query_num;
	struct {
		char *name;
		char *value;
	} *query;
	char *fragment;
} rtl_url_field_t;

int rtl_host_is_ipv4(const char *str);
rtl_url_field_t *rtl_url_parse(const char *str);
void rtl_url_free(rtl_url_field_t *url);
void rtl_url_field_print(rtl_url_field_t *url);
char *rtl_get_file_name_from_url(const char *url);

#endif /* _RTL_URL_H_ */
