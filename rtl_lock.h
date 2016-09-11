#ifndef _RTL_LOCK_H_
#define _RTL_LOCK_H_

#include <fcntl.h>

void rtl_lock_init(struct flock *lock, short type, short whence, off_t start, off_t len);
int rtl_readw_lock(int fd);
int rtl_writew_lock(int fd);
int rtl_unlock(int fd);
pid_t rtl_lock_test(int fd, short type, short whence, off_t start, off_t len);

#endif /* _RTL_LOCK_H_ */
