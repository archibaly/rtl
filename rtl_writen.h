#ifndef _RTL_WRITEN_H_
#define _RTL_WRITEN_H_

#include <unistd.h>

ssize_t rtl_writen(int fd, const void *vptr, size_t n);

#endif	/* _RTL_WRITEN_H_ */
