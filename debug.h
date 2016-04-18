#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

#include <stdio.h>

#define INFO(fmt, ...) fprintf(stdout, "%s(%d): " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define ERROR(fmt, ...) fprintf(stderr, "%s(%d): " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)


#else

#define INFO(...)
#define ERROR(...)

#endif

#endif /* _DEBUG_H_ */
