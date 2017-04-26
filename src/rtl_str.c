#include <string.h>
#include <ctype.h>

#include "rtl_str.h"

int rtl_streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0 ? 1 : 0;
}

char *rtl_strltrim(char *dst, const char *src, size_t siz)
{
	const char *p = src;

	if (!p)
		return NULL;
	while (isblank(*p))
		p++;
	if (rtl_strlcpy(dst, p, siz) >= siz)
		return NULL;
	return dst;
}

char *rtl_strrtrim(char *dst, const char *src, size_t siz)
{
	int pos, len;

	if (!src)
		return NULL;
	pos = strlen(src) - 1;
	while (isblank(src[pos]))
		pos--;
	len = pos + 1;
	if (siz < len + 1)
		return NULL;
	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}

char *rtl_strtrim(char *dst, const char *src, size_t siz)
{
	if (rtl_strltrim(dst, src, siz) < 0)
		return NULL;
	return rtl_strrtrim(dst, dst, siz);
}

char *rtl_strlower(char *dst, const char *src, size_t siz)
{
	int i, len;

	if (!src)
		return NULL;
	len = strlen(src);
	if (siz < len + 1)
		return NULL;
	for (i = 0; i < len; i++) {
		if (isupper(src[i]))
			dst[i] = src[i] + 32;
		else
			dst[i] = src[i];
	}
	dst[len] = '\0';
	return dst;
}

char *rtl_strupper(char *dst, const char *src, size_t siz)
{
	int i, len;

	if (!src)
		return NULL;
	len = strlen(src);
	if (siz < len + 1)
		return NULL;
	for (i = 0; i < len; i++) {
		if (islower(src[i]))
			dst[i] = src[i] - 32;
		else
			dst[i] = src[i];
	}
	dst[len] = '\0';
	return dst;
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

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return s - src - 1;	/* count does not include NUL */
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t rtl_strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return dlen + (s - src);	/* count does not include NUL */
}
