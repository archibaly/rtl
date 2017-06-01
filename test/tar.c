#include <stdio.h>
#include <rtl_tar.h>

int main()
{
	rtl_tar("-zcvf test.tgz test");
	rtl_tar("-zxvf test.tgz");
	return 0;
}
