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
	if (rtl_is_inner_ip("192.168.0.1"))
		printf("is inner ip\n");
	else
		printf("is not inner ip\n");

	return 0;
}
