#ifndef _RTL_CONFIG_H_
#define _RTL_CONFIG_H_

/* example: */
/* name = "jacky liu" */
/* age = 25 */

int rtl_config_load(const char *filename);
int rtl_config_save(const char *filename);
void rtl_config_free(void);
void rtl_config_set_delim(char d);
char *rtl_config_get_value(const char *name);
int rtl_config_set_value(const char *name, const char *value);
void rtl_config_print_opt(const char *name);

#endif /* _RTL_CONFIG_H_ */
