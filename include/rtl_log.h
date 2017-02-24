#ifndef _RTL_LOG_H_
#define _RTL_LOG_H_

#define RTL_LOG_ERR		1
#define RTL_LOG_WARNING	2
#define RTL_LOG_INFO	3
#define RTL_LOG_DEBUG	4

int rtl_log_open(const char *filename);
void rtl_log_set_max_size(int bytes);
void rtl_log_set_level(int level);
void rtl_log_write(int log_level, const char *fmt, ...);
void rtl_log_close(void);

#endif /* _RTL_LOG_H_ */
