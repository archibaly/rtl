#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include "send_file.h"

int send_file(const char *pathname, int out_fd)
{
	struct stat s;
	int fd = open(pathname, O_RDONLY);

	fstat(fd, &s);

	off_t nleft = s.st_size;
	int nwritten;

	while (nleft > 0) {
		if ((nwritten = sendfile(out_fd, fd, NULL, nleft)) < 0) {
			/* has sent all data */
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			} else {
				close(fd);
				return -1;
			}
		} else if (nwritten == 0) {
			close(fd);
			return -1;	/* error */
		}

		nleft -= nwritten;
	}

	close(fd);

	return s.st_size - nleft;
}
