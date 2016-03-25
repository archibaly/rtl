#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "time.h"
#include "lock.h"

static FILE *log_fp = NULL;
static log_level_t log_level = LOG_LEVEL_INFO;

int log_set_file(const char *filename)
{
	if ((log_fp = fopen(filename, "a")) == NULL)	
		return -1;
	return 0;
}

void log_set_level(log_level_t level)
{
	log_level = level;
}

static int log_level_string(log_level_t log_level, char *buf)
{
	int pos = 0;

	switch (log_level) {
		case LOG_LEVEL_INFO:
			strcpy(buf, "[INFO] ");
			pos = strlen("[INFO] ");
			break;
		case LOG_LEVEL_WARN:
			strcpy(buf, "[WARN] ");
			pos = strlen("[WARN] ");
			break;
		case LOG_LEVEL_ERRO:
			strcpy(buf, "[ERRO] ");
			pos = strlen("[ERRO] ");
			break;
		default:
			break;
	}
	return pos;
}

/*
 * format: [LOG_LEVEL] [time] message
 * example:
 *     [ERRO] [2015/03/25 14:58:03] some serious problem happened
 */
int log_write(log_level_t level, const char *fmt, ...)
{
	int pos;
	va_list params;
	char log_buf[1024];

	if (level < log_level)
		return 0;

	pos = log_level_string(level, log_buf);

	struct time time;
	time_get(&time);
	sprintf(log_buf + pos, "[%04d/%02d/%02d %02d:%02d:%02d] ",
			time.year, time.mon, time.day, time.hour, time.min, time.sec);
	pos += 22;

	va_start(params, fmt);
	vsnprintf(log_buf + pos, sizeof(log_buf) - 1, fmt, params);
	va_end(params);
	
	strcat(log_buf, "\n");

	int fd = fileno(log_fp);
	writew_lock(fd);
	fputs(log_buf, log_fp);
	fflush(log_fp);
	unlock(fd);

	return 0;
}
