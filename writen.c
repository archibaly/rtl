#include "writen.h"

/*
 * write "n" bytes to a descriptor
 */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	const char *ptr = vptr;

	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			return -1;
		} else if (nwritten == 0) {
			break;	/* has written all data */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return n - nleft;
}
