#include <stdio.h>
#include <string.h>

#include "rtl_fcgi.h"

int main()
{
	rtl_fcgi_t *fcgi;

	fcgi = rtl_fcgi_init("*", 2048);
	if (!fcgi)
		return 1;

	while (rtl_fcgi_accept(fcgi) >= 0) {
		printf("REMOTE_ADDR = %s\n", rtl_fcgi_getenv(fcgi, "REMOTE_ADDR"));
		printf("QUERY_STRING = %s\n", rtl_fcgi_getenv(fcgi, "QUERY_STRING"));

		int n, i;
		unsigned char *buf = rtl_fcgi_get_stdin(fcgi, &n);
		for (i = 0; i < n && buf; i++)
			printf("%c", buf[i]);
		printf("\n");

		char *html = "Content-Type: text/html\r\n\r\n<html>Hello World!</html>";
		rtl_fcgi_printf(fcgi, "%s", html);

		rtl_fcgi_finish(fcgi);
	}

	return 0;
}
