#include <stdio.h>
#include <string.h>
#include <time.h>

#include "time.h"

int time_get(struct time *t)
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

void time_fmt(const struct time *t, char *fmt, size_t size)
{
	snprintf(fmt, size - 1, "%04d/%02d/%02d %02d:%02d:%02d",
			 t->year, t->mon, t->mday, t->hour, t->min, t->sec);
}

void time_print(const struct time *t)
{
	printf("time.year = %d\n", t->year);
	printf("time.mon  = %d\n", t->mon);
	printf("time.mday = %d\n", t->mday);
	printf("time.wday = %d\n", t->wday);
	printf("time.hour = %d\n", t->hour);
	printf("time.min  = %d\n", t->min);
	printf("time.sec  = %d\n", t->sec);
}
