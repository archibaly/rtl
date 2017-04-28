#include <stdio.h>
#include <unistd.h>
#include <rtl_spt.h>

int main(int argc, char **argv)
{
	rtl_spt_init(argc, argv);
	rtl_setproctitle("spt_test");
	pause();
	return 0;
}
