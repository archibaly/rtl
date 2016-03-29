#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define INFO(str) fprintf(stdout, "%s\n", str)
#define ERROR(str) fprintf(stderr, str ": %s\n", strerror(errno))

#else

#define INFO(str)
#define ERROR(str)

#endif

#endif /* _DEBUG_H_ */
