#ifndef _RTL_ICONV_H_
#define _RTL_ICONV_H_

int rtl_iconv(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen);
int rtl_u2g(char *inbuf, int inlen, char *outbuf, int outlen);
int rtl_g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen);

#endif /* _RTL_ICONV_H_ */
