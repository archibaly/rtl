#ifndef _PID_H_
#define _PID_H_

#include <sys/types.h>

int find_pid_by_name(const char *pname, pid_t *pid, size_t size);

#endif /* _PID_H_ */
