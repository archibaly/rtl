#include <errno.h>

#include "rtl_readn.h"

/* read "n" bytes from a descriptor */
ssize_t rtl_readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nread == 0) {
			break;			/* EOF */
		}
		nleft -= nread;
		ptr   += nread;
	}

	return n - nleft;		/* return >= 0 */
}
