#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <rtl_http.h>
#include <rtl_https.h>
#include <rtl_debug.h>

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr, "%s [http|https] host port path\n", argv[0]);
		return -1;
	}

	char resp[81920];
	int ret;

	if (strcmp(argv[1], "http") == 0) {
		ret = rtl_http_send_request(RTL_HTTP_GET, argv[2], atoi(argv[3]), argv[4], resp, sizeof(resp));
	} else if (strcmp(argv[1], "https") == 0) {
		ret = rtl_https_send_request(RTL_HTTP_GET, argv[2], atoi(argv[3]), argv[4], resp, sizeof(resp));
	} else {
		return -1;
	}

	if (ret < 0) {
		rtl_debug("rtl_https_send_request error: %s", strerror(errno));
		return -1;
	}

	printf("%s\n", resp);

	return 0;
}
