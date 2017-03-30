#include <stdio.h>
#include <unistd.h>

#include <rtl_thread.h>

void *thread_func(rtl_thread_t *t)
{
	printf("name = %s\n", t->name);
	printf("args = %s\n", (char *)t->args);

	return NULL;
}

void *thread_func2(rtl_thread_t *t)
{
	printf("name = %s\n", t->name);
	printf("args = %s\n", (char *)t->args);

	for (;;) {
		printf("sleeping...\n");
		rtl_thread_cond_wait(t, 1000);
	}

	return NULL;
}

int main()
{
	rtl_thread_t *t1;

	t1 = rtl_thread_create(thread_func, "t1", "t1 test");
	rtl_thread_create(thread_func2, "t2", "t2 test");

	sleep(1);

	rtl_thread_destroy(t1);

	for (;;) {
		pause();
	}

	return 0;
}
