#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "uthash.h"

/* example: */
/* name = "jacky liu" */
/* age = 25 */

typedef struct {
	/* name of the option */
	char *name;
	/* value of the option */
	char *value;
	/* make this structure hashable */
	ut_hash_handle hh;
} config_opt_t;

int config_load(const char *);
int config_save(const char *);
void config_free(void);
void config_set_delim(char);
char *config_get_value(const char *);
void config_set_value(const char *, const char *);
void config_print_opt(const char *);

#endif /* _CONFIG_H_ */
