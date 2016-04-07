#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "time.h"
#include "lock.h"

#define LOG_MAX_SIZE	(1024 * 1024)	/* 1MB */

static struct log log = {NULL, -1, "", LOG_MAX_SIZE, LOG_LEVEL_INFO};

int log_open(const char *filename)
{
	if ((log.fp = fopen(filename, "a")) == NULL)
		return -1;
	strcpy(log.name, filename);
	log.fd = fileno(log.fp);
	return 0;
}

void log_set_max_size(int bytes)
{
	log.max_size = bytes;
}

void log_set_level(log_level_t level)
{
	log.level = level;
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

static void log_max_size_check(void)
{
	fseek(log.fp, 0, SEEK_END);
	if (ftell(log.fp) > log.max_size) {
		unlink(log.name);	/* just delete it */
		log_set_file_name(log.name);
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

	if (level < log.level)
		return 0;

	log_max_size_check();

	pos = log_level_string(level, log_buf);

	writew_lock(log.fd);

	char time[32];
	time_fmt(time, sizeof(time));
	sprintf(log_buf + pos, "[%s] ", time);
	pos = strlen(log_buf);

	va_start(args, fmt);
	vsnprintf(log_buf + pos, sizeof(log_buf) - 1, fmt, args);
	va_end(args);
	
	strcat(log_buf, "\n");

	fputs(log_buf, log.fp);
	fflush(log.fp);

	unlock(log.fd);

	return 0;
}

void log_close(void)
{
	fclose(log.fp);
	log.fp = NULL;
	log.fd = -1;
	log.max_size = LOG_MAX_SIZE;
	log.level = LOG_LEVEL_INFO;
}
