#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>   /* NULL size_t */
#include <stdarg.h>   /* va_list va_start va_end */
#include <stdlib.h>   /* malloc(3) setenv(3) clearenv(3) setproctitle(3) getprogname(3) */
#include <stdio.h>    /* vsnprintf(3) snprintf(3) */
#include <string.h>   /* strlen(3) strchr(3) strdup(3) memset(3) memcpy(3) */
#include <errno.h>    /* errno program_invocation_name program_invocation_short_name */

#if !defined(HAVE_SETPROCTITLE)
#define HAVE_SETPROCTITLE (defined __NetBSD__ || defined __FreeBSD__ || defined __OpenBSD__)
#endif


#if !HAVE_SETPROCTITLE
#if (defined __linux || defined __APPLE__)

extern char **environ;

static struct {
	/* original value */
	const char *arg0;
	/* title space available */
	char *base, *end;
	/* pointer to original nul character within base */
	char *nul;
	_Bool reset;
	int error;
} spt;


#ifndef SPT_MIN
#define SPT_MIN(a, b) (((a) < (b))? (a) : (b))
#endif

static inline size_t spt_min(size_t a, size_t b);
static int spt_clearenv(void);
static int spt_copyenv(char *oldenv[]);

static int spt_copyargs(int argc, char *argv[]) ;
void spt_init(int argc, char *argv[]);
void setproctitle(const char *fmt, ...);


static inline size_t spt_min(size_t a, size_t b)
{
	return SPT_MIN(a, b);
}


/*
 * For discussion on the portability of the various methods, see
 * http://lists.freebsd.org/pipermail/freebsd-stable/2008-June/043136.html
 */
static int spt_clearenv(void)
{
#if __GLIBC__
	clearenv();

	return 0;
#else
	extern char **environ;
	static char **tmp;

	if (!(tmp = malloc(sizeof *tmp)))
		return errno;

	tmp[0]  = NULL;
	environ = tmp;

	return 0;
#endif
}


static int spt_copyenv(char *oldenv[])
{
	extern char **environ;
	char *eq;
	int i, error;

	if (environ != oldenv)
		return 0;

	if ((error = spt_clearenv()))
		goto error;

	for (i = 0; oldenv[i]; i++) {
		if (!(eq = strchr(oldenv[i], '=')))
			continue;

		*eq = '\0';
		error = (0 != setenv(oldenv[i], eq + 1, 1))? errno : 0;
		*eq = '=';

		if (error)
			goto error;
	}

	return 0;
error:
	environ = oldenv;

	return error;
}


static int spt_copyargs(int argc, char *argv[])
{
	char *tmp;
	int i;

	for (i = 1; i < argc || (i >= argc && argv[i]); i++) {
		if (!argv[i])
			continue;

		if (!(tmp = strdup(argv[i])))
			return errno;

		argv[i] = tmp;
	}

	return 0;
}


void rtl_spt_init(int argc, char *argv[])
{
	char **envp = environ;
	char *base, *end, *nul;
	int i, error;

	if (!(base = argv[0]))
		return;

	nul = &base[strlen(base)];
	end = nul + 1;

	for (i = 0; i < argc || (i >= argc && argv[i]); i++) {
		if (!argv[i] || argv[i] < end)
			continue;

		end = argv[i] + strlen(argv[i]) + 1;
	}

	for (i = 0; envp[i]; i++) {
		if (envp[i] < end)
			continue;

		end = envp[i] + strlen(envp[i]) + 1;
	}

	if (!(spt.arg0 = strdup(argv[0])))
		goto syerr;

	if ((error = spt_copyenv(envp)))
		goto error;

	if ((error = spt_copyargs(argc, argv)))
		goto error;

	spt.nul  = nul;
	spt.base = base;
	spt.end  = end;

	return;
syerr:
	error = errno;
error:
	spt.error = error;
}


#ifndef SPT_MAXTITLE
#define SPT_MAXTITLE 255
#endif

void rtl_setproctitle(const char *fmt, ...)
{
	char buf[SPT_MAXTITLE + 1]; /* use buffer in case argv[0] is passed */
	va_list ap;
	char *nul;
	int len, error;

	if (!spt.base)
		return;

	if (fmt) {
		va_start(ap, fmt);
		len = vsnprintf(buf, sizeof buf, fmt, ap);
		va_end(ap);
	} else {
		len = snprintf(buf, sizeof buf, "%s", spt.arg0);
	}

	if (len <= 0) {
		error = errno;
		goto error;
	}

	if (!spt.reset) {
		memset(spt.base, 0, spt.end - spt.base);
		spt.reset = 1;
	} else {
		memset(spt.base, 0, spt_min(sizeof buf, spt.end - spt.base));
	}

	len = spt_min(len, spt_min(sizeof buf, spt.end - spt.base) - 1);
	memcpy(spt.base, buf, len);
	nul = &spt.base[len];

	if (nul < spt.nul) {
		*spt.nul = '.';
	} else if (nul == spt.nul && &nul[1] < spt.end) {
		*spt.nul = ' ';
		*++nul = '\0';
	}

	return;
error:
	spt.error = error;
}

#endif /* __linux || __APPLE__ */
#endif /* !HAVE_SETPROCTITLE */
