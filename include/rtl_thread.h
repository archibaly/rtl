#ifndef _RTL_THREAD_H_
#define _RTL_THREAD_H_

#include <stdint.h>
#include <pthread.h>

#include "rtl_lock.h"

typedef struct rtl_thread rtl_thread_t;

struct rtl_thread {
	pthread_t tid;
	rtl_spin_lock_t *spin;
	rtl_mutex_lock_t *mutex;
	rtl_mutex_cond_t *cond;
	rtl_sem_lock_t *sem;
	char name[32];
	void *(*func)(rtl_thread_t *);
	void *args;
};

rtl_thread_t *rtl_thread_create(void *(*func)(rtl_thread_t *),
		const char *name, void *args);

void rtl_thread_destroy(rtl_thread_t *t);

int rtl_thread_spin_lock(rtl_thread_t *t);
int rtl_thread_spin_unlock(rtl_thread_t *t);

int rtl_thread_sem_wait(rtl_thread_t *t, int64_t ms);
int rtl_thread_sem_signal(rtl_thread_t *t);

int rtl_thread_mutex_lock(rtl_thread_t *t);
int rtl_thread_mutex_unlock(rtl_thread_t *t);

int rtl_thread_cond_wait(rtl_thread_t *t, int64_t ms);
int rtl_thread_cond_signal(rtl_thread_t *t);
int rtl_thread_cond_signal_all(rtl_thread_t *t);

#endif /* _RTL_THREAD_H_ */
