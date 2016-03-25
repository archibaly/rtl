#ifndef _CONFIG_H_
#define _CONFIG_H_

int config_get(const char *filename, const char *name, char *value);
int config_set(const char *filename, const char *name, const char *value);

#endif /* _CONFIG_H_ */
