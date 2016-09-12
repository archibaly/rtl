#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sem.h>

#include "rtl_sem.h"
#include "rtl_debug.h"

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

/* init semaphore by semctl */
int rtl_set_semvalue(int sem_id, int value)
{
	union semun sem_union;

	sem_union.val = value;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
		rtl_debug("semctl error: %s", strerror(errno));
		return -1;
	}

	return 0;
}


/* delete semaphore by sectl */
int rtl_del_semvalue(int sem_id)
{
	union semun sem_union;

	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1) {
		rtl_debug("semctl error: %s", strerror(errno));
		return -1;
	}
	return 0;
}

/* p(v) */
int rtl_sem_p(int sem_id)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;	/* P(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_b, 1) == -1) {
		rtl_debug("semop error: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/* v(v) */
int rtl_sem_v(int sem_id)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;	/* V(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_b, 1) == -1) {
		rtl_debug("semop error: %s", strerror(errno));
		return -1;
	}

	return 0;
}
