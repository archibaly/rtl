#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include "send_file.h"

int send_file(const char *pathname, int out_fd)
{
	int fd;
	struct stat s;

	if ((fd = open(pathname, O_RDONLY)) < 0)
		return -1;

	if (fstat(fd, &s) < 0) {
		close(fd);
		return -1;
	}

	off_t nleft = s.st_size;
	int nwritten;

	while (nleft > 0) {
		if ((nwritten = sendfile(out_fd, fd, NULL, nleft)) < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			} else {
				close(fd);
				return -1;
			}
		} else if (nwritten == 0) {	/* has sent all data */
			break;
		}

		nleft -= nwritten;
	}

	close(fd);

	return s.st_size - nleft;
}
