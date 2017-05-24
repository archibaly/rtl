#ifndef _RTL_SIGNAL_H_
#define _RTL_SIGNAL_H_

#include <signal.h>

typedef void (*sighandler_t) (int);

sighandler_t rtl_signal(int signo, sighandler_t func);

#endif /* _RTL_SIGNAL_H_ */
