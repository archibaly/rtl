#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>

#include <rtl_http.h>
#include <rtl_https.h>

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr, "%s [http|https] path host port\n", argv[0]);
		return -1;
	}

	char resp[BUFSIZ * 2];
	struct rtl_http_connection *hc;
	struct rtl_https_connection *hsc;

	if (strcmp(argv[1], "http") == 0) {
		hc = rtl_http_send_get_request(argv[2], argv[3], atoi(argv[4]), 0);
		if (!hc) {
			fprintf(stderr, "rtl_https_send_request error: %s\n", strerror(errno));
			return -1;
		}
	} else if (strcmp(argv[1], "https") == 0) {
		hsc = rtl_https_send_get_request(argv[2], argv[3], atoi(argv[4]), 0);
		if (!hsc) {
			fprintf(stderr, "rtl_https_send_request error: %s\n", strerror(errno));
			return -1;
		}
	} else {
		return -1;
	}

	if (strcmp(argv[1], "http") == 0) {
		int i, n;
		for (i = 0; ; i += n) {
			n = rtl_http_recv_response(hc, (uint8_t *)resp + i, sizeof(resp) - i);
			printf("read %d bytes\n", n);
			if (n < 0)
				exit(1);
			else if (n == 0)
				break;
		}
		/* rtl_http_save_body_to_file(hc, "body"); */
		rtl_http_close_connection(hc);
	} else {
		rtl_https_recv_response(hsc, (uint8_t *)resp, sizeof(resp));
		/* rtl_https_save_body_to_file(hsc, "body"); */
		rtl_https_close_connection(hsc);
	}

	printf("%s\n", resp);

	return 0;
}
