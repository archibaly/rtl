#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "time.h"

#define LOG_MAX_NAME_SIZE	64
#define LOG_MAX_FILE_SIZE	(1024 * 1024)	/* 1MB */

struct log {
	FILE *fp;
	char name[LOG_MAX_NAME_SIZE];
	int max_size;
	log_level_t level;
};

static struct log log = {NULL, "", LOG_MAX_FILE_SIZE, LOG_DEBG};

int log_open(const char *filename)
{
	if (!(log.fp = fopen(filename, "a")))
		return -1;
	strncpy(log.name, filename, sizeof(log.name) - 1);
	log.name[sizeof(log.name) - 1] = '\0';
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
	case LOG_ERRO:
		strcpy(buf, "[ERRO]");
		pos = strlen("[ERRO]");
		break;
	case LOG_WARN:
		strcpy(buf, "[WARN]");
		pos = strlen("[WARN]");
		break;
	case LOG_INFO:
		strcpy(buf, "[INFO]");
		pos = strlen("[INFO]");
		break;
	case LOG_DEBG:
		strcpy(buf, "[DEBG]");
		pos = strlen("[DEBG]");
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
		log_close();
		log_open(log.name);
	}
}

/*
 * format: [LOG_LEVEL][time] message
 * example:
 *     [ERRO][2015/03/25 14:58:03] some serious problem happened
 */
void log_write(log_level_t level, const char *fmt, ...)
{
	int pos;
	va_list args;
	char log_buf[1024];

	if (level > log.level || !log.fp)
		return;

	if (access(log.name, 0) < 0)
		if (log_open(log.name) < 0)
			return;

	log_max_size_check();

	pos = log_level_string(level, log_buf);

	char now[32];
	time_fmt(now, sizeof(now), "%Y/%m/%d %H:%M:%S");
	sprintf(log_buf + pos, "[%s] ", now);
	pos = strlen(log_buf);

	va_start(args, fmt);
	vsnprintf(log_buf + pos, sizeof(log_buf) - 1, fmt, args);
	va_end(args);
	
	strcat(log_buf, "\n");
	fputs(log_buf, log.fp);
	fflush(log.fp);
}

void log_close(void)
{
	if (log.fp) {
		fclose(log.fp);
		log.fp = NULL;
	}
}
