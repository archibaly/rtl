#include <stdio.h>
#include <string.h>
#include <time.h>

#include "time.h"

void time_get(struct time *t)
{
	time_t ticks = time(NULL);
	struct tm *pt = localtime(&ticks);

	t->year = pt->tm_year + 1900;
	t->mon =  pt->tm_mon + 1;
	t->day = pt->tm_mday;
	t->hour = pt->tm_hour;
	t->min = pt->tm_min;
	t->sec = pt->tm_sec;
}

void time_fmt(char *fmt, int size)
{
	struct time time;
	time_get(&time);
	snprintf(fmt, size, "%04d/%02d/%02d %02d:%02d:%02d",
			 time.year, time.mon, time.day, time.hour, time.min, time.sec);
}
