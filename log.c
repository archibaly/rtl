#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "time.h"
#include "lock.h"

static FILE *log_fp = NULL;
static int log_max_line = 2048;
static int current_line = 0;
static log_level_t log_level = LOG_LEVEL_INFO;

int log_set_file(const char *filename)
{
	if ((log_fp = fopen(filename, "w")) == NULL)
		return -1;
	return 0;
}

/*
 * set max number of line in log file
 */
int log_set_max_line(int n)
{
	log_max_line = n;
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

static void log_max_line_check(FILE *fp)
{
	current_line++;
	if (current_line > log_max_line) {
		current_line = 0;
		rewind(log_fp);
	}
}

/*
 * format: [LOG_LEVEL] [time] message
 * example:
 *     [ERRO] [2015/03/25 14:58:03] some serious problem happened
 */
int log_write(log_level_t level, const char *fmt, ...)
{
	int pos;
	va_list args;
	char log_buf[1024];

	if (level < log_level)
		return 0;

	pos = log_level_string(level, log_buf);

	int fd = fileno(log_fp);
	writew_lock(fd);

	struct time time;
	time_get(&time);
	sprintf(log_buf + pos, "[%04d/%02d/%02d %02d:%02d:%02d] ",
			time.year, time.mon, time.day, time.hour, time.min, time.sec);
	pos += 22;

	va_start(args, fmt);
	vsnprintf(log_buf + pos, sizeof(log_buf) - 1, fmt, args);
	va_end(args);
	
	strcat(log_buf, "\n");

	log_max_line_check(log_fp);
	fputs(log_buf, log_fp);
	fflush(log_fp);
	unlock(fd);

	return 0;
}
