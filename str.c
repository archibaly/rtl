#include <string.h>

#include "str.h"

char *trim(char *str)
{
	char *tail, *head;

	for (tail = str + strlen(str) - 1; tail >= str; tail--)
		if (!ISSPACE(*tail))
			break;
	tail[1] = 0;
	for (head = str; head <= tail; head++)
		if (!ISSPACE(*head))
			break;
	if (head != str)
		memcpy(str, head, (tail - head + 2) * sizeof(char));

	return str;
}

int streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0 ? 1 : 0;
}

int is_lower(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return 1;
	return 0;
}

int is_upper(char ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return 1;
	return 0;
}

char *strupper(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; i++)
		if (is_lower(str[i]))
			str[i] -= 32;
	str[len] = '\0';
	return str;
}

char *strlower(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; i++)
		if (is_upper(str[i]))
			str[i] += 32;
	str[len] = '\0';
	return str;
}
