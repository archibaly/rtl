#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "http.h"
#include "https.h"
#include "socket.h"

/*
 * @host: the remote host's name or ip
 * @port: the remote host port
 * @path: the path to get
 * @resp: response buffer
 * @len: the length of response buffer
 */
int https_send_get(const char *host, uint16_t port, const char *path, char *resp, int len)
{
	char header[512];

	int header_len = http_build_get_header(host, path, header);

    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        return -1;
    }

	SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) {
        return -1;
    }
	
	int sockfd = socket_connect(host, port);
	if (sockfd < 0)
		return -1;

    int ret = SSL_set_fd(ssl, sockfd);
    if (ret == 0) {
        return -1;
    }

    ret = SSL_connect(ssl);
    if (ret != 1) {
        return -1;
    }

	int nleft = header_len;
	int nwritten;
	char *ptr = header;
	while (nleft > 0) {
		if ((nwritten = SSL_write(ssl, ptr, nleft)) < 0) {
			/* has written all data */
			break;
		} else if (nwritten == 0) {
			return -1;	/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	/* read response */
	int nread;
	ptr = resp;
	for (;;) {
		nread = SSL_read(ssl, ptr, len);
		if (nread == 0) {	/* receive done */
			break;
		} else if (nread < 0) {
			return -1;
		}
		len -= nread;
		ptr += nread;
		if (len < 0)
			return -1;
	}
	nread = ptr - resp;
	ptr[nread] = '\0';

    SSL_shutdown(ssl);
    close(sockfd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

	return nread;
}
