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
} rtl_time_t;

int rtl_time_get(rtl_time_t *t);
int rtl_time_fmt(char *s, size_t size, const char *fmt);
int rtl_time_mono(void);

#endif /* _RTL_TIME_H_ */
