#include <stdio.h>
#include <string.h>

#include <rtl_str.h>

int main()
{
	char *str = "   abc   ";
	char buf[16];

	printf("left trim buf = (%s)\n", rtl_strltrim(buf, str, sizeof(buf)));
	printf("right trim buf = (%s)\n", rtl_strrtrim(buf, str, sizeof(buf)));
	printf("trim buf = (%s)\n", rtl_strtrim(buf, str, sizeof(buf)));
	printf("toupper buf = (%s)\n", rtl_strupper(buf, str, sizeof(buf)));
	printf("tolower buf = (%s)\n", rtl_strlower(buf, str, sizeof(buf)));

	return 0;
}
