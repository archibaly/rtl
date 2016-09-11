#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

#include "rtl_pid.h"

#define READ_BUF_SIZE	512

/*
 * @pname: process name
 * @pid: array for pid
 * @size: the size of array
 * @return: the number of found pid
 */
int rtl_find_pid_by_name(const char *pname, pid_t *pid, size_t size)
{
	DIR *dir;
	struct dirent *next;
	int i = 0;

	if (!(dir = opendir("/proc")))
		return 0;

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* if it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (!(status = fopen(filename, "r"))) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE - 1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* buffer should contain a string like "name binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(pname, name) == 0) {
			if (i < size)
				pid[i++] = strtol(next->d_name, NULL, 0);
			else
				break;
		}
	}

	closedir(dir);

	return i;
}
