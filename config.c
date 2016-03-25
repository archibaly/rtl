#include <stdio.h>
#include <string.h>

#include "config.h"
#include "kmp.h"
#include "lock.h"
#include "str.h"

#define DELIM	'='

static int found(char *line, const char *name)
{
	trim(line);
	if (kmp(line, name) == 0) {
		if (ISSPACE(line[strlen(name)]) || line[strlen(name)] == DELIM) {
			int i = strlen(name);
			while (line[i] != DELIM)
				i++;
			i++;
			while (ISSPACE(line[i]))
				i++;
			return i;
		}
	}
	return 0;
}

int config_get(const char *filename, const char *name, char *value)
{
	FILE *fp;
	int ret = -1;

    if ((fp = fopen(filename, "r")) == NULL)
		goto end;

	int fd = fileno(fp);
	readw_lock(fd);
	int pos;
	char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
		if ((pos = found(line, name))) {
			strncpy(value, line + pos, strlen(line + pos) - 1);
			ret = 0;
		}
	}
	unlock(fd);

	fclose(fp);

end:
    return ret;
}

int config_set(const char *filename, const char *name, const char *value)
{
	FILE *fp;
	FILE *_fp;
	char line[1024];
	char _filename[256];
	int ret = -1;

    if ((fp = fopen(filename, "r")) == NULL)
		goto end;

	sprintf(_filename, "%s.bak", filename);
    if ((_fp = fopen(_filename, "w")) == NULL)
		goto end;

	int fd = fileno(fp);
	int _fd = fileno(_fp);
	readw_lock(fd);
	writew_lock(_fd);
    while (fgets(line, sizeof(line), fp) != NULL) {
		if (found(line, name)) {
			sprintf(line, "%s = %s\n", name, value);
			ret = 0;
		}
		fputs(line, _fp);
	}

	rename(_filename, filename);
	unlock(fd);
	unlock(_fd);

	fclose(fp);
	fclose(_fp);

end:
    return ret; 
}
