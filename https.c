#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "https.h"
#include "socket.h"

/*
 * @type: GET or POST
 * @host: the remote host's name or ip
 * @port: the remote host port
 * @path: the path to get
 * @resp: response buffer
 * @len: the length of response buffer
 */
int https_send_request(enum request type, const char *host, uint16_t port, const char *path, char *resp, int len)
{
	char header[512];
	int header_len;

	if (type == GET)
		header_len = http_build_get_header(host, path, header);
	else if (type == POST)
		header_len = http_build_post_header(host, path, header);
	else
		return -1;

    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL)
        return -1;

	int ret = 0;
	SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) {
		ret = -1;
		goto out;
	}
	
	int sockfd = socket_connect(host, port);
	if (sockfd < 0) {
		ret = -1;
		goto out;
	}

    if (SSL_set_fd(ssl, sockfd) == 0) {
		ret = -1;
		goto out;
	}

    if (SSL_connect(ssl) != 1) {
		ret = -1;
		goto out;
    }

	int nleft = header_len;
	int nwritten;
	char *ptr = header;
	while (nleft > 0) {
		if ((nwritten = SSL_write(ssl, ptr, nleft)) <= 0) {
			ret = -1;
			goto out;
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
			ret = -1;
			goto out;
		}
		len -= nread;
		ptr += nread;
		if (len < 0) {	/* the buffer of resp is not enough */
			ret = -1;
			goto out;
		}
	}
	ret = ptr - resp;

out:
    SSL_shutdown(ssl);
    close(sockfd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

	return ret;
}
