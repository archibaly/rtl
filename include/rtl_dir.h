#ifndef _RTL_DIR_H_
#define _RTL_DIR_H_

#include <sys/stat.h>
#include <sys/types.h>

int rtl_mkdir(const char *path, mode_t omode);
int rtl_unlink(const char *name);

#endif /* _RTL_DIR_H_ */
