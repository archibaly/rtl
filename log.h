#ifndef _LOG_H_
#define _LOG_H_

typedef enum {
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_ERRO = 3
} log_level_t;

int log_set_file(const char *filename);
void log_set_level(log_level_t level);
int log_write(log_level_t log_level, const char *fmt, ...);

#endif /* _LOG_H_ */
