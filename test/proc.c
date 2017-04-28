#include <stdio.h>
#include <unistd.h>

#include <rtl_proc.h>

void proc1(void *args)
{
	for (;;) {
		printf("proc1 executed\n");
		sleep(1);
	}
}

void proc2(void *args)
{
	for (;;) {
		printf("proc2 executed\n");
		sleep(2);
	}
}

int main()
{
	rtl_proc_spawn(proc1, NULL, "proc1", 0);
	rtl_proc_spawn(proc2, NULL, "proc2", 0);
	rtl_proc_wait();
	return 0;
}
