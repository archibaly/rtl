#ifndef _RTL_SETPROCTITLE_H_
#define _RTL_SETPROCTITLE_H_

void rtl_spt_init(int argc, char *argv[]);
void rtl_setproctitle(const char *fmt, ...);

#endif /* _RTL_SETPROCTITLE_H_ */

