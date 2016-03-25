#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int unlink_r(const char* name)
{
	struct stat st;
	DIR *dir;
	struct dirent *de;
	int fail = 0;

	/* is it a file or directory? */
	if (lstat(name, &st) < 0)
		return -1;

	/* a file, so unlink it */
	if (!S_ISDIR(st.st_mode))
		return unlink(name);

	/* a directory, so open handle */
	dir = opendir(name);
	if (dir == NULL)
		return -1;

	/* recurse over components */
	errno = 0;
	while ((de = readdir(dir)) != NULL) {
		char dn[PATH_MAX];
		if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, "."))
			continue;
		sprintf(dn, "%s/%s", name, de->d_name);
		if (unlink_recursive(dn) < 0) {
			fail = 1;
			break;
		}
		errno = 0;
	}
	/* in case readdir or unlink_recursive failed */
	if (fail || errno < 0) {
		int save = errno;
		closedir(dir);
		errno = save;
		return -1;
	}

	/* close directory handle */
	if (closedir(dir) < 0)
		return -1;

	/* delete target directory */
	return rmdir(name);
}
