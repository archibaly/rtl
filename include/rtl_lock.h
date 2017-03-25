#ifndef _RTL_LOCK_H_
#define _RTL_LOCK_H_

#include <stdint.h>

/*
 * spin lock implemented by atomic APIs
 */
typedef struct {
	int lock;
	int ncpu;
} rtl_spin_lock_t;
rtl_spin_lock_t *rtl_spin_lock_init();
int rtl_spin_lock(rtl_spin_lock_t *lock);
int rtl_spin_unlock(rtl_spin_lock_t *lock);
int rtl_spin_trylock(rtl_spin_lock_t *lock);
void rtl_spin_lock_deinit(rtl_spin_lock_t *lock);


/*
 * mutex lock implemented by pthread_mutex APIs
 */
typedef void rtl_mutex_lock_t;
rtl_mutex_lock_t *rtl_mutex_lock_init();
int rtl_mutex_trylock(rtl_mutex_lock_t *lock);
int rtl_mutex_lock(rtl_mutex_lock_t *lock);
int rtl_mutex_unlock(rtl_mutex_lock_t *lock);
void rtl_mutex_lock_deinit(rtl_mutex_lock_t *lock);


/*
 * external APIs of mutex condition
 */
typedef void rtl_mutex_cond_t;
rtl_mutex_cond_t *rtl_mutex_cond_init();
int rtl_mutex_cond_wait(rtl_mutex_lock_t *mutex, rtl_mutex_cond_t *cond, int64_t ms);
void rtl_mutex_cond_signal(rtl_mutex_cond_t *cond);
void rtl_mutex_cond_signal_all(rtl_mutex_cond_t *cond);
void rtl_mutex_cond_deinit(rtl_mutex_cond_t *cond);


/*
 * read-write lock implemented by pthread_rtl_rwlock APIs
 */
typedef void rtl_rwlock_t;
rtl_rwlock_t *rtl_rwlock_init();
int rtl_rwlock_rdlock(rtl_rwlock_t *lock);
int rtl_rwlock_tryrdlock(rtl_rwlock_t *lock);
int rtl_rwlock_wrlock(rtl_rwlock_t *lock);
int rtl_rwlock_trywrlock(rtl_rwlock_t *lock);
int rtl_rwlock_unlock(rtl_rwlock_t *lock);
void rtl_rwlock_deinit(rtl_rwlock_t *lock);


/*
 * sem lock implemented by Unnamed semaphores (memory-based semaphores) APIs
 */
typedef void rtl_sem_lock_t;
rtl_sem_lock_t *rtl_sem_lock_init();
int rtl_sem_lock_wait(rtl_sem_lock_t *lock, int64_t ms);
int rtl_sem_lock_trywait(rtl_sem_lock_t *lock);
int rtl_sem_lock_signal(rtl_sem_lock_t *lock);
void rtl_sem_lock_deinit(rtl_sem_lock_t *lock);

#endif /* _RTL_LOCK_H_ */
