#include <stdio.h>

#include <rtl_inet.h>

int main()
{
	if (rtl_enable_promisc("eth0") < 0) {
		perror("rtl_enable_promisc");
		return 1;
	}
	if (rtl_disable_promisc("eth0") < 0) {
		perror("rtl_enable_promisc");
		return 1;
	}
	return 0;
}
