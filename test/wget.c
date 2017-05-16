#include <stdio.h>

#include <rtl_wget.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s url\n", argv[0]);
		return -1;
	}

	return rtl_wget(argv[1], "wget.log");
}
