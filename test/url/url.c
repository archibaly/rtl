#include <stdio.h>
#include <rtl_url.h>

int main()
{
	char *str = "http://222.73.136.209:8085/clientcert/config_1980895021.tgz";

	rtl_url_field_t *url = rtl_url_parse(str);

	printf("schema: %s\n", url->schema);
	printf("host: %s\n", url->host);
	printf("port: %s\n", url->port);
	printf("path: %s\n", url->path);

	rtl_url_free(url);

	return 0;
}
