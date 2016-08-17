#ifndef _CONFIG_H_
#define _CONFIG_H_

/* example: */
/* name = "jacky liu" */
/* age = 25 */

typedef struct {
	char *name;
	char *value;
} config_opt_t;

int config_load(const char *);
int config_save(const char *);
void config_free(void);
void config_set_delim(char);
char *config_get_value(const char *);
void config_set_value(const char *, const char *);
void config_print_opt(const char *);

#endif /* _CONFIG_H_ */
