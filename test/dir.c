#include <stdio.h>

#include <rtl_dir.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "mkdir <path>\n");
		return 1;
	}

	if (rtl_dir_create(argv[1], 0755) < 0) {
		perror("rtl_dir_create");
		return 1;
	}

	if (rtl_dir_remove(argv[1]) < 0) {
		perror("rtl_dir_remove");
		return 1;
	}

	return 0;
}

