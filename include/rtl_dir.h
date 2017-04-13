#ifndef _RTL_DIR_H_
#define _RTL_DIR_H_

#include <sys/stat.h>
#include <sys/types.h>

int rtl_dir_create(const char *path, mode_t omode);
int rtl_dir_remove(const char *name);

#endif /* _RTL_DIR_H_ */
