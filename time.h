#ifndef _TIME_H_
#define _TIME_H_

struct time {
	int year;
	int mon;
	int day;
	int hour;
	int min;
	int sec;
};

void time_get(struct time *t);
void time_fmt(char *fmt, int size);

#endif /* _TIME_H_ */
