#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>

#include "rtl_lock.h"

/* spin lock APIs */
#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define cpu_pause() __asm__ ("pause")
#else
#define cpu_pause()
#endif

#define atomic_cmp_set(lock, old, set) \
	__sync_bool_compare_and_swap(lock, old, set)

rtl_spin_lock_t *rtl_spin_lock_init(void)
{
	rtl_spin_lock_t *lock = (rtl_spin_lock_t *)calloc(1, sizeof(rtl_spin_lock_t));
	if (!lock) {
		fprintf(stderr, "malloc rtl_spin_lock_t failed:%d\n", errno);
		return NULL;
	}
	lock->ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	return lock;
}

int rtl_spin_lock(rtl_spin_lock_t *lock)
{
	int spin = 2048;
	int value = 1;
	int i, n;
	for ( ;; ) {
		if (lock->lock == 0 && atomic_cmp_set(&lock->lock, 0, value)) {
			return 0;
		}
		if (lock->ncpu > 1) {
			for (n = 1; n < spin; n <<= 1) {
				for (i = 0; i < n; i++) {
					cpu_pause();
				}
				if (lock->lock == 0 && atomic_cmp_set(&lock->lock, 0, value)) {
					return 0;
				}
			}
		}
		sched_yield();
	}
	return 0;
}

int rtl_spin_unlock(rtl_spin_lock_t *lock)
{
	lock->lock = 0;
	return 0;
}

int rtl_spin_trylock(rtl_spin_lock_t *lock)
{
	return (lock->lock == 0 && atomic_cmp_set(&lock->lock, 0, 1));
}

void rtl_spin_lock_deinit(rtl_spin_lock_t *lock)
{
	if (!lock) {
		return;
	}
	free(lock);
}

/* mutex lock APIs */
rtl_mutex_lock_t *rtl_mutex_lock_init(void)
{
	pthread_mutex_t *lock = (pthread_mutex_t *)calloc(1,
			sizeof(pthread_mutex_t));
	if (!lock) {
		fprintf(stderr, "malloc pthread_mutex_t failed:%d\n", errno);
		return NULL;
	}
	pthread_mutex_init(lock, NULL);
	return lock;
}

void rtl_mutex_lock_deinit(rtl_mutex_lock_t *ptr)
{
	if (!ptr) {
		return;
	}
	pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
	int ret = pthread_mutex_destroy(lock);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "the mutex is currently locked.\n");
				break;
			default:
				fprintf(stderr, "pthread_mutex_trylock error:%s.\n", strerror(ret));
				break;
		}
	}
	free(lock);
}

int rtl_mutex_trylock(rtl_mutex_lock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
	int ret = pthread_mutex_trylock(lock);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "the mutex could not be acquired"
						" because it was currently locked.\n");
				break;
			case EINVAL:
				fprintf(stderr, "the mutex has not been properly initialized.\n");
				break;
			default:
				fprintf(stderr, "pthread_mutex_trylock error:%s.\n", strerror(ret));
				break;
		}
	}
	return ret;
}

int rtl_mutex_lock(rtl_mutex_lock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
	int ret = pthread_mutex_lock(lock);
	if (ret != 0) {
		switch (ret) {
			case EDEADLK:
				fprintf(stderr, "the mutex is already locked by the calling thread"
						" (``error checking'' mutexes only).\n");
				break;
			case EINVAL:
				fprintf(stderr, "the mutex has not been properly initialized.\n");
				break;
			default:
				fprintf(stderr, "pthread_mutex_trylock error:%s.\n", strerror(ret));
				break;
		}
	}
	return ret;
}

int rtl_mutex_unlock(rtl_mutex_lock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_mutex_t *lock = (pthread_mutex_t *)ptr;
	int ret = pthread_mutex_unlock(lock);
	if (ret != 0) {
		switch (ret) {
			case EPERM:
				fprintf(stderr, "the calling thread does not own the mutex"
						" (``error checking'' mutexes only).\n");
				break;
			case EINVAL:
				fprintf(stderr, "the mutex has not been properly initialized.\n");
				break;
			default:
				fprintf(stderr, "pthread_mutex_trylock error:%s.\n", strerror(ret));
				break;
		}
	}
	return ret;
}

rtl_mutex_cond_t *rtl_mutex_cond_init(void)
{
	pthread_cond_t *cond = (pthread_cond_t *)calloc(1, sizeof(pthread_cond_t));
	if (!cond) {
		fprintf(stderr, "malloc pthread_cond_t failed:%d\n", errno);
		return NULL;
	}
	/* never return an error code */
	pthread_cond_init(cond, NULL);
	return cond;
}

void rtl_mutex_cond_deinit(rtl_mutex_cond_t *ptr)
{
	if (!ptr) {
		return;
	}
	pthread_cond_t *cond = (pthread_cond_t *)ptr;
	int ret = pthread_cond_destroy(cond);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "some threads are currently waiting on cond.\n");
				break;
			default:
				fprintf(stderr, "pthread_cond_destroy error:%s.\n", strerror(ret));
				break;
		}
	}
	free(cond);
}

int rtl_mutex_cond_wait(rtl_mutex_lock_t *mutexp, rtl_mutex_cond_t *condp, int64_t ms)
{
	if (!condp || !mutexp) {
		return -1;
	}
	int ret = 0;
	int retry = 3;
	pthread_mutex_t *mutex = (pthread_mutex_t *)mutexp;
	pthread_cond_t *cond = (pthread_cond_t *)condp;
	if (ms <= 0) {
		/* never return an error code */
		pthread_cond_wait(cond, mutex);
	} else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		uint64_t ns = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
		ns += ms * 1000 * 1000;
		ts.tv_sec = ns / (1000 * 1000 * 1000);
		ts.tv_nsec = ns % 1000 * 1000 * 1000;
wait:
		ret = pthread_cond_timedwait(cond, mutex, &ts);
		if (ret != 0) {
			switch (ret) {
				case ETIMEDOUT:
					fprintf(stderr, "the condition variable was not signaled "
							"until the timeout specified by abstime.\n");
					break;
				case EINTR:
					fprintf(stderr, "pthread_cond_timedwait was interrupted by a signal.\n");
					if (--retry != 0) {
						goto wait;
					}
					break;
				default:
					fprintf(stderr, "pthread_cond_timedwait error:%s.\n", strerror(ret));
					break;
			}
		}
	}
	return ret;
}

void rtl_mutex_cond_signal(rtl_mutex_cond_t *ptr)
{
	if (!ptr) {
		return;
	}
	pthread_cond_t *cond = (pthread_cond_t *)ptr;
	/* never return an error code */
	pthread_cond_signal(cond);
}

void rtl_mutex_cond_signal_all(rtl_mutex_cond_t *ptr)
{
	if (!ptr) {
		return;
	}
	pthread_cond_t *cond = (pthread_cond_t *)ptr;
	/* never return an error code */
	pthread_cond_broadcast(cond);
}

/* read-write lock APIs */
rtl_rwlock_t *rtl_rwlock_init(void)
{
	pthread_rwlock_t *lock = (pthread_rwlock_t *)calloc(1,
			sizeof(pthread_rwlock_t));
	if (!lock) {
		fprintf(stderr, "malloc pthread_rwlock_t failed:%d\n", errno);
		return NULL;
	}
	int ret = pthread_rwlock_init(lock, NULL);
	if (ret != 0) {
		switch (ret) {
			case EAGAIN:
				fprintf(stderr, "The system lacked the necessary resources (other than"
						" memory) to initialize another read-write lock.\n");
				break;
			case ENOMEM:
				fprintf(stderr, "Insufficient memory exists to initialize the "
						"read-write lock.\n");
				break;
			case EPERM:
				fprintf(stderr, "The caller does not have the privilege to perform "
						"the operation.\n");
				break;
			default:
				fprintf(stderr, "pthread_rwlock_init failed:%d\n", ret);
				break;
		}
		free(lock);
		lock = NULL;
	}

	return lock;
}

void rtl_rwlock_deinit(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	if (0 != pthread_rwlock_destroy(lock)) {
		fprintf(stderr, "pthread_rwlock_destroy failed!\n");
	}
	free(lock);
}

int rtl_rwlock_rdlock(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	int ret = pthread_rwlock_rdlock(lock);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "The read-write lock could not be acquired for reading "
						"because a writer holds the lock or a writer with the "
						"appropriate priority was blocked on it.\n");
				break;
			case EAGAIN:
				fprintf(stderr, "The read lock could not be acquired because the maximum "
						"number of read locks for rtl_rwlock has been exceeded.\n");
				break;
			case EDEADLK:
				fprintf(stderr, "A deadlock condition was detected or the current thread "
						"already owns the read-write lock for writing.\n");
				break;
			default:
				fprintf(stderr, "pthread_rwlock_destroy failed!\n");
				break;
		}
	}
	return ret;
}

int rtl_rwlock_tryrdlock(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	int ret = pthread_rwlock_tryrdlock(lock);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "The read-write lock could not be acquired for reading "
						"because a writer holds the lock or a writer with the "
						"appropriate priority was blocked on it.\n");
				break;
			case EAGAIN:
				fprintf(stderr, "The read lock could not be acquired because the maximum "
						"number of read locks for rtl_rwlock has been exceeded.\n");
				break;
			case EDEADLK:
				fprintf(stderr, "A deadlock condition was detected or the current thread "
						"already owns the read-write lock for writing.\n");
				break;
			default:
				fprintf(stderr, "pthread_rwlock_tryrdlock failed!\n");
				break;
		}
	}
	return ret;
}

int rtl_rwlock_wrlock(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	int ret = pthread_rwlock_wrlock(lock);
	if (ret != 0) {
		switch (ret) {
			case EDEADLK:
				fprintf(stderr, "A deadlock condition was detected or the current thread "
						"already owns the read-write lock for writing or reading.\n");
				break;
			default:
				fprintf(stderr, "pthread_rwlock_wrlock failed!\n");
				break;
		}
	}
	return ret;
}

int rtl_rwlock_trywrlock(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	int ret = pthread_rwlock_trywrlock(lock);
	if (ret != 0) {
		switch (ret) {
			case EBUSY:
				fprintf(stderr, "The read-write lock could not be acquired for writing "
						"because it was already locked for reading or writing.\n");
				break;

			default:
				fprintf(stderr, "pthread_rwlock_trywrlock failed!\n");
				break;
		}
	}
	return ret;
}

int rtl_rwlock_unlock(rtl_rwlock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	pthread_rwlock_t *lock = (pthread_rwlock_t *)ptr;
	int ret = pthread_rwlock_unlock(lock);
	if (ret != 0) {
		switch (ret) {
			default:
				fprintf(stderr, "pthread_rwlock_unlock failed!\n");
				break;
		}
	}
	return ret;
}

/* sem lock APIs */
rtl_sem_lock_t *rtl_sem_lock_init(void)
{
	rtl_sem_lock_t *lock = (rtl_sem_lock_t *)calloc(1, sizeof(rtl_sem_lock_t));
	if (!lock) {
		fprintf(stderr, "malloc rtl_sem_lock_t failed:%d\n", errno);
		return NULL;
	}

	/* 0: threads, 1: processes */
	int pshared = 0;
	if (0 != sem_init(lock, pshared, 0)) {
		fprintf(stderr, "rtl_sem_init failed %d:%s\n", errno, strerror(errno));
		free(lock);
		lock = NULL;
	}
	return lock;
}

void rtl_sem_lock_deinit(rtl_sem_lock_t *ptr)
{
	if (!ptr) {
		return;
	}
	rtl_sem_lock_t *lock = (rtl_sem_lock_t *)ptr;
	if (0 != sem_destroy(lock)) {
		fprintf(stderr, "rtl_sem_destroy %d:%s\n", errno , strerror(errno));
	}
	free(lock);
}

int rtl_sem_lock_wait(rtl_sem_lock_t *ptr, int64_t ms)
{
	if (!ptr) {
		return -1;
	}
	int ret;
	rtl_sem_lock_t *lock = (rtl_sem_lock_t *)ptr;
	if (ms < 0) {
		ret = sem_wait(lock);
		if (ret != 0) {
			switch (errno) {
				case EINTR:
					fprintf(stderr, "The call was interrupted by a signal handler.\n");
					break;
				case EINVAL:
					fprintf(stderr, "sem is not a valid semaphore.\n");
					break;
			}
		}
	} else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		uint64_t ns = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
		ns += ms * 1000 * 1000;
		ts.tv_sec = ns / (1000 * 1000 * 1000);
		ts.tv_nsec = ns % 1000 * 1000 * 1000;
		ret = sem_timedwait(lock, &ts);
		if (ret != 0) {
			switch (errno) {
				case EINVAL:
					fprintf(stderr, "The value of abs_timeout.tv_nsecs is less than 0, "
							"or greater than or equal to 1000 million.\n");
					break;
				case ETIMEDOUT:
					fprintf(stderr, "The call timed out before the semaphore could be locked.\n");
					break;
			}
		}
	}
	return ret;
}

int rtl_sem_lock_trywait(rtl_sem_lock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	int ret;
	rtl_sem_lock_t *lock = (rtl_sem_lock_t *)ptr;
	ret = sem_trywait(lock);
	if (ret != 0) {
		switch (errno) {
			case EAGAIN:
				fprintf(stderr, "The operation could not be performed without blocking\n");
				break;
		}
	}
	return ret;
}

int rtl_sem_lock_signal(rtl_sem_lock_t *ptr)
{
	if (!ptr) {
		return -1;
	}
	int ret;
	rtl_sem_lock_t *lock = (rtl_sem_lock_t *)ptr;
	ret = sem_post(lock);
	if (ret != 0) {
		switch (errno) {
			case EINVAL:
				fprintf(stderr, "sem is not a valid semaphore.\n");
				break;
			case EOVERFLOW:
				fprintf(stderr, "The maximum allowable value for a semaphore would be exceeded.\n");
				break;
		}
	}
	return ret;
}
