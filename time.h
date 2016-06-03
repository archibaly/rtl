#ifndef _TIME_H_
#define _TIME_H_

struct time {
	int year;
	int mon;
	int mday;
	int wday;
	int hour;
	int min;
	int sec;
};

int time_get(struct time *t);
void time_fmt(const struct time *time, char *fmt, int size);

#endif /* _TIME_H_ */
