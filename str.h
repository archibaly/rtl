#ifndef _STR_H_
#define _STR_H_

#define ISSPACE(x)	((x)==' '||(x)=='\t'||(x)=='\n')

char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *str);
int streq(const char *a, const char *b);
char *strlower(char *str);
char *strupper(char *str);
size_t strlcpy(char *dst, const char *src, size_t siz);

#endif /* _STR_H_ */
