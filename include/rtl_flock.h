#ifndef _RTL_LOCK_H_
#define _RTL_LOCK_H_

#include <fcntl.h>

void rtl_flock_init(struct flock *lock, short type, short whence, off_t start, off_t len);
int rtl_file_read_lock(int fd);
int rtl_file_write_lock(int fd);
int rtl_file_unlock(int fd);
pid_t rtl_flock_test(int fd, short type, short whence, off_t start, off_t len);

#endif /* _RTL_LOCK_H_ */
