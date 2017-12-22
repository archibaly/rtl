#ifndef _RTL_LOG_H_
#define _RTL_LOG_H_

#include <syslog.h>

#define	RTL_LOG_EMERG	LOG_EMERG
#define	RTL_LOG_ALERT	LOG_ALERT
#define	RTL_LOG_CRIT	LOG_CRIT
#define	RTL_LOG_ERR		LOG_ERR
#define	RTL_LOG_WARNING	LOG_WARNING
#define	RTL_LOG_NOTICE	LOG_NOTICE
#define	RTL_LOG_INFO	LOG_INFO
#define	RTL_LOG_DEBUG	LOG_DEBUG

int rtl_log_open(const char *filename);
void rtl_log_set_max_size(int bytes);
void rtl_log_set_level(int level);
void rtl_log_write(int log_level, const char *fmt, ...);
void rtl_log_close(void);

#endif /* _RTL_LOG_H_ */
