#include <stdio.h>
#include <unistd.h>

#include <rtl_thread.h>

void *thread_func(rtl_thread_t *t)
{
	printf("name = %s\n", t->name);
	printf("args = %s\n", (char *)t->args);

	return NULL;
}

int main()
{
	rtl_thread_t *t1;
	rtl_thread_t *t2;

	t1 = rtl_thread_create(thread_func, "t1", "t1 test");
	t2 = rtl_thread_create(thread_func, "t2", "t2 test");

	sleep(1);

	rtl_thread_destroy(t1);
	rtl_thread_destroy(t2);

	return 0;
}
