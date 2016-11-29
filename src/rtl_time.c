#include <time.h>

#include "rtl_time.h"

int rtl_time_get(rtl_time_t *t)
{
	time_t ticks = time(NULL);
	if (ticks < 0)
		return -1;

	struct tm tm;
	if (!localtime_r(&ticks, &tm))
		return -1;

	t->year = tm.tm_year + 1900;
	t->mon =  tm.tm_mon + 1;
	t->mday = tm.tm_mday;
	t->wday = tm.tm_wday;
	t->hour = tm.tm_hour;
	t->min = tm.tm_min;
	t->sec = tm.tm_sec;

	return 0;
}

int rtl_time_fmt(char *s, size_t size, const char *fmt)
{
	time_t ticks = time(NULL);
	if (ticks < 0)
		return -1;

	struct tm tm;
	if (!localtime_r(&ticks, &tm))
		return -1;

	strftime(s, size, fmt, &tm);

	return 0;
}

int rtl_time_mono(void)
{
	struct timespec t;

	if (clock_gettime(CLOCK_MONOTONIC, &t) < 0)
		return -1;

	return t.tv_sec;
}
