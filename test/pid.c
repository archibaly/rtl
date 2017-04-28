#include <stdio.h>
#include <sys/types.h>

#include "rtl_pid.h"

int main()
{
	pid_t pid;

	rtl_find_pid_by_name("systemd", &pid, 1);
	printf("systemd pid = %d\n", pid);

	rtl_save_pid_to_file("pid.pid");
	printf("%d\n", rtl_read_pid_from_file("pid.pid"));

	return 0;
}
