#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define ERROR(fmt) fprintf(stderr, fmt ": %s\n", strerror(errno))

#else

#define ERROR(...)

#endif

#endif /* _DEBUG_H_ */
