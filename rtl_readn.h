#ifndef _RTL_READN_H_
#define _RTL_READN_H_

#include <unistd.h>

ssize_t rtl_readn(int fd, void *vptr, size_t n);

#endif /* _RTL_READN_H_ */
