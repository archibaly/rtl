#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <rtl_http.h>
#include <rtl_https.h>
#include <rtl_debug.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr, "%s [http|https] path host port\n", argv[0]);
		return -1;
	}

	char resp[81920];
	struct ssl ssl;
	struct rtl_socket_connection *sc;

	if (strcmp(argv[1], "http") == 0) {
		sc = rtl_http_send_get_request(argv[2], argv[3], atoi(argv[4]));
		if (!sc) {
			rtl_debug("rtl_https_send_request error: %s", strerror(errno));
			return -1;
		}
	} else if (strcmp(argv[1], "https") == 0) {
		if (rtl_https_send_get_request(&ssl, argv[2], argv[3], atoi(argv[4])) < 0) {
			rtl_debug("rtl_https_send_request error: %s", strerror(errno));
			return -1;
		}
	} else {
		return -1;
	}


	if (strcmp(argv[1], "http") == 0) {
		rtl_http_recv_response(sc, (uint8_t *)resp, sizeof(resp));
		/* rtl_http_save_body_to_file(sc, "body"); */
	} else {
		rtl_https_recv_response(&ssl, (uint8_t *)resp, sizeof(resp));
		/* rtl_https_save_body_to_file(&ssl, "body"); */
	}

	printf("%s\n", resp);

	return 0;
}
