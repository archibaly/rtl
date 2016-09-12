#ifndef _RTL_DEBUG_H_
#define _RTL_DEBUG_H_

#ifdef DEBUG
#include <stdio.h>
#define rtl_debug(fmt, ...) \
	fprintf(stderr, "%s(%d): " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define rtl_debug(fmt, ...)
#endif

#endif /* _RTL_DEBUG_H_ */
