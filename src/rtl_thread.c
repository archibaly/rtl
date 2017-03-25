#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "rtl_thread.h"

static void *__thread_func(void *args)
{
	rtl_thread_t *t = (rtl_thread_t *)args;
	if (!t->func) {
		fprintf(stderr, "rtl_thread_t function is null\n");
		return NULL;
	}
	t->func(t);
	return NULL;
}

rtl_thread_t *rtl_thread_create(void *(*func)(rtl_thread_t *),
		const char *name, void *args)
{
	rtl_thread_t *t = calloc(1, sizeof(rtl_thread_t));
	if (!t) {
		fprintf(stderr, "calloc rtl_thread_t failed(%d): %s\n", errno, strerror(errno));
		goto err;
	}
	if (!(t->spin = rtl_spin_lock_init())) {
		fprintf(stderr, "rtl_spin_lock_init failed\n");
		goto err;
	}
	if (!(t->mutex = rtl_mutex_lock_init())) {
		fprintf(stderr, "rtl_mutex_lock_init failed\n");
		goto err;
	}
	if (!(t->cond = rtl_mutex_cond_init())) {
		fprintf(stderr, "rtl_mutex_cond_init failed\n");
		goto err;
	}
	if (!(t->sem = rtl_sem_lock_init())) {
		fprintf(stderr, "rtl_sem_lock_init failed\n");
		goto err;
	}

	strncpy(t->name, name, sizeof(t->name) - 1);
	t->args = args;
	t->func = func;
	if (0 != pthread_create(&t->tid, NULL, __thread_func, t)) {
		fprintf(stderr, "pthread_create failed(%d): %s\n", errno, strerror(errno));
		goto err;
	}
	return t;

err:
	if (t->spin) rtl_spin_lock_deinit(t->spin);
	if (t->sem) rtl_sem_lock_deinit(t->sem);
	if (t->mutex) rtl_mutex_lock_deinit(t->mutex);
	if (t->cond) rtl_mutex_cond_deinit(t->cond);
	if (t) {
		free(t);
	}
	return NULL;
}

void rtl_thread_destroy(rtl_thread_t *t)
{
	if (!t) {
		return;
	}
	if (t->spin) rtl_spin_lock_deinit(t->spin);
	if (t->sem) rtl_sem_lock_deinit(t->sem);
	if (t->mutex) rtl_mutex_lock_deinit(t->mutex);
	if (t->cond) rtl_mutex_cond_deinit(t->cond);
	pthread_join(t->tid, NULL);
	free(t);
}

int rtl_thread_spin_lock(rtl_thread_t *t)
{
	if (!t || !t->spin) {
		return -1;
	}
	return rtl_spin_lock(t->spin);
}

int rtl_thread_spin_unlock(rtl_thread_t *t)
{
	if (!t || !t->spin) {
		return -1;
	}
	return rtl_spin_unlock(t->spin);
}

int rtl_thread_mutex_lock(rtl_thread_t *t)
{
	if (!t || !t->mutex) {
		return -1;
	}
	return rtl_mutex_lock(t->mutex);
}

int rtl_thread_mutex_unlock(rtl_thread_t *t)
{
	if (!t || !t->mutex) {
		return -1;
	}
	return rtl_mutex_unlock(t->mutex);
}

int rtl_thread_cond_wait(rtl_thread_t *t, int64_t ms)
{
	if (!t || !t->mutex || !t->cond) {
		return -1;
	}
	return rtl_mutex_cond_wait(t->mutex, t->cond, ms);
}

int rtl_thread_cond_signal(rtl_thread_t *t)
{
	if (!t || !t->cond) {
		return -1;
	}
	rtl_mutex_cond_signal(t->cond);
	return 0;
}

int rtl_thread_cond_signal_all(rtl_thread_t *t)
{
	if (!t || !t->cond) {
		return -1;
	}
	rtl_mutex_cond_signal_all(t->cond);
	return 0;
}

int rtl_thread_sem_wait(rtl_thread_t *t, int64_t ms)
{
	if (!t || !t->sem) {
		return -1;
	}
	return rtl_sem_lock_wait(t->sem, ms);
}

int rtl_thread_sem_signal(rtl_thread_t *t)
{
	if (!t || !t->sem) {
		return -1;
	}
	return rtl_sem_lock_signal(t->sem);
}
