#ifndef _RTL_TIME_H_
#define _RTL_TIME_H_

typedef struct {
	int year;
	int mon;
	int mday;
	int wday;
	int hour;
	int min;
	int sec;
} rtl_time;

int rtl_time_get(rtl_time *t);
int rtl_time_fmt(char *s, size_t size, const char *fmt);

#endif /* _RTL_TIME_H_ */
