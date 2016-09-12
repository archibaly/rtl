#include <ctype.h>
#include <string.h>

#include "rtl_str.h"

char *rtl_ltrim(char *s)
{
	char *p = s;
	while (RTL_ISSPACE(*p))
		p++;
	strcpy(s, p);
	return s;
}

char *rtl_rtrim(char *s)
{
	int i;

	i = strlen(s) - 1;
	while (RTL_ISSPACE(s[i]))
		i--;
	s[i + 1] = '\0';

	return s;
}

char *rtl_trim(char *s)
{
	rtl_ltrim(s);
	rtl_rtrim(s);
	return s;
}

int rtl_streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0 ? 1 : 0;
}

char *rtl_strlower(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; i++)
		if (isupper(str[i]))
			str[i] += 32;
	str[len] = '\0';
	return str;
}

char *rtl_strupper(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; i++)
		if (islower(str[i]))
			str[i] -= 32;
	str[len] = '\0';
	return str;
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t rtl_strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';      /* NUL-terminate dst */
		while (*s++)
			;
	}

	return s - src - 1;    /* count does not include NUL */
}
