#ifndef _RTL_PID_H_
#define _RTL_PID_H_

#include <stddef.h>
#include <sys/types.h>

int rtl_find_pid_by_name(const char *pname, pid_t *pid, size_t size);
int rtl_save_pid_to_file(const char *filename);
pid_t rtl_read_pid_from_file(const char *filename);

#endif /* _RTL_PID_H_ */
