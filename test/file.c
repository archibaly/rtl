#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>

#include <rtl_file.h>

static void foo(void)
{
	int i = 0;
	rtl_file_backend_type_t type;
	char buf[128] = {0};
	for (i = 0; i < 2; ++i) {
		if (i == 0)
			type = RTL_FILE_BACKEND_IO;
		else if (i == 1)
			type = RTL_FILE_BACKEND_FIO;
		rtl_file_backend(type);
		printf("backend = %d\n", type);
		struct rtl_file *fw = rtl_file_open("/tmp/lsusb", RTL_F_RDWR);
		printf("fw = %p\n", fw);
		if (!fw)
			continue;
		rtl_file_write(fw, "hello file\n", 11);
		rtl_file_sync(fw);
		rtl_file_seek(fw, 0, SEEK_SET);
		rtl_file_read(fw, buf, sizeof(buf));
		printf("buf = %s", buf);
		rtl_file_close(fw);

		struct rtl_file *f = rtl_file_open("/tmp/lsusb", RTL_F_RDONLY);
		printf("f = %p\n", f);
		if (!f)
			continue;
		rtl_file_read(f, buf, sizeof(buf));
		printf("buf = %s", buf);
		printf("len = %zu\n", rtl_file_get_size("/tmp/lsusb"));
		struct iovec *iobuf = rtl_file_dump("/tmp/lsusb");
		if (iobuf) {
			printf("len = %zu, buf = %s\n", iobuf->iov_len, (char *)iobuf->iov_base);
		}
	}
}

static void foo2(void)
{
	struct rtl_file_systat *stat = rtl_file_get_systat("/var/run/crond.pid");
	if (!stat)
		return;
	printf("total = %zuMB\n", stat->size_total/(1024*1024));
	printf("avail = %zuMB\n", stat->size_avail/(1024*1024));
	printf("free = %zuMB\n", stat->size_free/(1024*1024));
	printf("fs type name = %s\n", stat->fs_type_name);
	free(stat);
}

static void foo3(void)
{
	printf("local path = %s\n", rtl_file_path_pwd());
	printf("suffix = %s\n", rtl_file_path_suffix(rtl_file_path_pwd()));
	printf("prefix = %s\n", rtl_file_path_prefix(rtl_file_path_pwd()));
}

int main(int argc, char **argv)
{
	foo();
	foo2();
	foo3();
	return 0;
}
