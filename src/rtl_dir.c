#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "rtl_dir.h"

int rtl_dir_create(const char *path, mode_t omode)
{
	if (mkdir(path, omode) < 0) {
		if (errno == EEXIST)
			return -1;
	} else {
		return 0;
	}

	struct stat sb;
	mode_t numask, oumask = 0;
	int first, last, retval = 0;
	char *cp = strdup(path);
	char *p = cp;
	char *save = cp;

	if (!cp)
		return -1;

	if (p[0] == '/')		/* Skip leading '/'. */
		++p;

	for (first = 1, last = 0; !last ; ++p) {
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (!last && p[1] == '\0')
			last = 1;
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those cased by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(S_IWUSR | S_IXUSR);
			(void)umask(numask);
			first = 0;
		}
		if (last)
			(void)umask(oumask);
		if (mkdir(cp, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(cp, &sb) < 0) {
					retval = -1;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last)
						errno = EEXIST;
					else
						errno = ENOTDIR;
					retval = -1;
					break;
				}
			} else {
				retval = -1;
				break;
			}
		}
		if (!last)
			*p = '/';
	}
	free(save);

	if (!first && !last)
		(void)umask(oumask);

	return retval;
}

int rtl_dir_remove(const char *name)
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
		if (rtl_dir_remove(dn) < 0) {
			fail = 1;
			break;
		}
		errno = 0;
	}
	/* in case readdir or rtl_unlink failed */
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
