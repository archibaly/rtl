#include <stdio.h>
#include <string.h>

#include "rtl_fcgi.h"

int main()
{
	rtl_fcgi_t *fcgi;

	fcgi = rtl_fcgi_init("*", 2048);
	if (!fcgi)
		return 1;

	int ret;

	for (;;) {
		rtl_fcgi_accept(fcgi);
		ret = rtl_fcgi_read_request(fcgi);

		printf("rtl_fcgi_read_request return: %d\n", ret);

		printf("REMOTE_ADDR = %s\n", rtl_fcgi_getenv(fcgi, "REMOTE_ADDR"));

		char *html = "Content-Type: text/html\r\n\r\n<html>Hello World!</html>";
		/* rtl_fcgi_write(fcgi, html, strlen(html)); */
		rtl_fcgi_printf(fcgi, "%s", html);

		rtl_fcgi_finish(fcgi);
	}

	return 0;
}
