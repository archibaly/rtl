#include <stdio.h>
#include <rtl_url.h>

int main()
{
	char *str = "http://222.73.136.209:8085/clientcert/config_1980895021.tgz";

	printf("%s\n", rtl_url_get_file_name(str));

	rtl_url_field_t *url = rtl_url_parse(str);

	rtl_url_field_print(url);

	rtl_url_free(url);

	return 0;
}
