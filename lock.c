#include <unistd.h>

#include "lock.h"

void lock_init(struct flock *lock, short type, short whence, off_t start, off_t len)
{
	if (lock == NULL)
		return;

	lock->l_type = type;
	lock->l_whence = whence;
	lock->l_start = start;
	lock->l_len = len;
}

int readw_lock(int fd)
{
	if (fd < 0)
		return -1;

	struct flock lock;
	lock_init(&lock, F_RDLCK, SEEK_SET, 0, 0);

	if (fcntl(fd, F_SETLKW, &lock) != 0)
		return -1;

	return 0;
}

int writew_lock(int fd)
{
	if (fd < 0)
		return -1;

	struct flock lock;
	lock_init(&lock, F_WRLCK, SEEK_SET, 0, 0);

	if (fcntl(fd, F_SETLKW, &lock) != 0)
		return -1;

	return 0;
}

int unlock(int fd)
{
	if (fd < 0)
		return -1;

	struct flock lock;
	lock_init(&lock, F_UNLCK, SEEK_SET, 0, 0);

	if (fcntl(fd, F_SETLKW, &lock) != 0)
		return -1;

	return 0;
}

pid_t lock_test(int fd, short type, short whence, off_t start, off_t len)
{
	struct flock lock;
	lock_init(&lock, type, whence, start, len);

	if (fcntl(fd, F_GETLK, &lock) == -1)
		return -1;

	if (lock.l_type == F_UNLCK)
		return 0;
	return lock.l_pid;
}
