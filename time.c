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
