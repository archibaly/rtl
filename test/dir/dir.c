#include <stdio.h>

#include <rtl_dir.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "mkdir <path>\n");
		return 1;
	}

	if (rtl_mkdir(argv[1], 0755) < 0) {
		perror("rtl_mkdir");
		return 1;
	}

	return 0;
}

