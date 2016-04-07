#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

#include <stdio.h>

#define INFO(...) fprintf(stdout, __VA_ARGS__)
#define ERROR(...) fprintf(stderr, __VA_ARGS__)

#else

#define INFO(...)
#define ERROR(...)

#endif

#endif /* _DEBUG_H_ */
