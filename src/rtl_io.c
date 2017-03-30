#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "rtl_file.h"

#define MAX_RETRY_CNT	3

static struct rtl_file_desc *io_open(const char *path, rtl_file_open_mode_t mode)
{
	struct rtl_file_desc *file = (struct rtl_file_desc *)calloc(1,
			sizeof(struct rtl_file_desc));
	if (!file) {
		fprintf(stderr, "malloc failed:%d %s\n", errno, strerror(errno));
		return NULL;
	}
	int flags = -1;
	switch(mode) {
	case RTL_F_RDONLY:
		flags = O_RDONLY;
		break;
	case RTL_F_WRONLY:
		flags = O_WRONLY;
		break;
	case RTL_F_RDWR:
		flags = O_RDWR;
		break;
	case RTL_F_CREATE:
		flags = O_WRONLY | O_CREAT;
		break;
	case RTL_F_WRCLEAR:
		flags = O_WRONLY | O_TRUNC;
		break;
	default:
		fprintf(stderr, "unsupport file mode!\n");
		break;
	}
	file->fd = open(path, flags, 0666);
	if (file->fd < 0) {
		fprintf(stderr, "open %s failed:%d %s\n", path, errno, strerror(errno));
		free(file);
		return NULL;
	}
	if (!(file->name = strdup(path))) {
		close(file->fd);
		free(file);
		return NULL;
	}
	return file;
}

static ssize_t io_read(struct rtl_file_desc *file, void *buf, size_t len)
{
	int n;
	char *p = (char *)buf;
	size_t left = len;
	size_t step = 1024*1024;
	int cnt = 0;
	if (file == NULL || buf == NULL || len == 0) {
		fprintf(stderr, "%s paraments invalid!\n", __func__);
		return -1;
	}
	int fd = file->fd;
	while (left > 0) {
		if (left < step)
			step = left;
		n = read(fd, (void *)p, step);
		if (n > 0) {
			p += n;
			left -= n;
			continue;
		} else {
			if (n == 0) {
				break;
			} else {
				if (errno == EINTR || errno == EAGAIN) {
					if (++cnt > MAX_RETRY_CNT)
						break;
					continue;
				}
			}
		}
	}
	return len - left;
}

static ssize_t io_write(struct rtl_file_desc *file, const void *buf, size_t count)
{
	ssize_t n;
	char *cursor = (char *)buf;
	int retry = 0;
	if (file == NULL || buf == NULL || count == 0) {
		fprintf(stderr, "%s paraments invalid, "
#if __WORDSIZE == 64
				"count=%lu!\n",
#else
				"count=%u!\n",
#endif
				__func__, count);
		return -1;
	}
	int fd = file->fd;
	size_t remain = count;
	size_t step = 1024 * 1024;
	while (remain > 0) {
		if (remain < step)
			step = remain;
		n = write(fd, (void *)cursor, step);
		if (n > 0) {
			cursor += n;
			remain -= n;
			continue;
		} else {
			if (errno == EINTR || errno == EAGAIN) {
				if (++retry > MAX_RETRY_CNT) {
					fprintf(stderr, "reach max retry\n");
					break;
				}
				continue;
			} else {
				fprintf(stderr, "write failed:%d %s\n", errno, strerror(errno));
				break;
			}
		}
	}
	return count - remain;
}

static off_t io_seek(struct rtl_file_desc *file, off_t offset, int whence)
{
	if (!file)
		return -1;
	return lseek(file->fd, offset, whence);
}

static int io_sync(struct rtl_file_desc *file)
{
	if (!file)
		return -1;
	return fsync(file->fd);
}

static size_t io_size(struct rtl_file_desc *file)
{
	struct stat buf;
	if (stat(file->name, &buf) < 0)
		return 0;
	return (size_t)buf.st_size;
}

static void io_close(struct rtl_file_desc *file)
{
	if (file) {
		close(file->fd);
		free(file->name);
		free(file);
	}
}

struct rtl_file_ops rtl_io_ops = {
	.open  = io_open,
	.write = io_write,
	.read  = io_read,
	.seek  = io_seek,
	.sync  = io_sync,
	.size  = io_size,
	.close = io_close,
};
