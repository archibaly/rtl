#ifndef _RTL_STR_H_
#define _RTL_STR_H_

#define RTL_ISSPACE(x)	((x)==' '||(x)=='\t'||(x)=='\n')

char *rtl_ltrim(char *s);
char *rtl_rtrim(char *s);
char *rtl_trim(char *str);
int rtl_streq(const char *a, const char *b);
char *rtl_strlower(char *str);
char *rtl_strupper(char *str);
size_t rtl_strlcpy(char *dst, const char *src, size_t siz);

#endif /* _RTL_STR_H_ */
