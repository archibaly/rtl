#ifndef _RTL_STR_H_
#define _RTL_STR_H_

int rtl_streq(const char *a, const char *b);
char *rtl_strltrim(char *dst, const char *src, size_t siz);
char *rtl_strrtrim(char *dst, const char *src, size_t siz);
char *rtl_strtrim(char *dst, const char *src, size_t siz);
char *rtl_strlower(char *dst, const char *src, size_t siz);
char *rtl_strupper(char *dst, const char *src, size_t siz);
size_t rtl_strlcpy(char *dst, const char *src, size_t siz);
size_t rtl_strlcat(char *dst, const char *src, size_t siz);

#endif /* _RTL_STR_H_ */
