#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <rtl_shm.h>
#include <unistd.h>

rtl_shm_ctl_block_t *scb;

void signal_handler(int signo)
{
	rtl_shm_sem_del(scb);
	rtl_shm_mem_del(scb);
	_exit(0);
}

int main()
{
	char *tmp;

	signal(SIGINT, signal_handler);

	scb = rtl_shm_init(1024 * 10);
	if (!scb)
		return -1;

	for (;;) {
		tmp = rtl_shm_malloc(scb, 131);
		if (tmp) {
			strncpy(tmp, "malloc succeed!", 130);
			printf("%s\n", tmp);
			rtl_shm_free(scb, tmp);
		} else {
			printf("%s\n", "malloc failed!");
		}
	}
}
