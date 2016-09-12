#ifndef _RTL_SIGNAL_H_
#define _RTL_SIGNAL_H_

#include <signal.h>

__sighandler_t rtl_signal(int signo, __sighandler_t func);

#endif /* _RTL_SIGNAL_H_ */
