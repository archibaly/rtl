#ifndef _LOCK_H_
#define _LOCK_H_

#include <fcntl.h>

void lock_init(struct flock *lock, short type, short whence, off_t start, off_t len);
int readw_lock(int fd);
int writew_lock(int fd);
int unlock(int fd);
pid_t lock_test(int fd, short type, short whence, off_t start, off_t len);

#endif /* _LOCK_H_ */
