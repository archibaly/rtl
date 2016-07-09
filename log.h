#ifndef _LOG_H_
#define _LOG_H_

typedef enum {
	LOG_ERRO = 1,
	LOG_WARN = 2,
	LOG_INFO = 3,
	LOG_DEBG = 4
} log_level_t;

int log_open(const char *filename);
void log_set_max_size(int bytes);
void log_set_level(log_level_t level);
void log_write(log_level_t log_level, const char *fmt, ...);
void log_close(void);

#endif /* _LOG_H_ */
