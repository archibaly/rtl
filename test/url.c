#include <stdio.h>
#include <rtl_url.h>

int main()
{
	char *str = "http://222.73.136.209:8085/clientcert/config_1980895021.tgz";

	printf("%s\n", rtl_url_get_file_name(str));

	rtl_url_field_t *url = rtl_url_parse(str);

	rtl_url_field_print(url);

	rtl_url_free(url);

	char qs[] = "username=jacky&passwd=123456&ip=192.168.20.234";

	char *kvpairs[8];
	int n = rtl_url_query_parse(qs, kvpairs, sizeof(kvpairs) / sizeof(kvpairs[0]));
	printf("number of query is: %d\n", n);
	printf("username = %s\n", rtl_url_query_k2v("username", kvpairs, n));
	printf("passwd = %s\n", rtl_url_query_k2v("passwd", kvpairs, n));

	return 0;
}
