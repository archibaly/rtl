#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DEBUG

#include <stdio.h>

#define debug(fmt, ...) fprintf(stderr, "%s(%d): " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)

#else

#define debug(fmt, ...)

#endif

#endif /* _DEBUG_H_ */
