#include <string.h>
#include <iconv.h>

#include "rtl_iconv.h"

int rtl_iconv(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0)
		return -1;

	int ret = 0;

	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1) {
		ret = -1;
		goto out;
	}

out:
	iconv_close(cd);
	return ret;
}

int rtl_u2g(char *inbuf, int inlen, char *outbuf, int outlen)
{
	return rtl_iconv("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}

int rtl_g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return rtl_iconv("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}
