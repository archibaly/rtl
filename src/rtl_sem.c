#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "rtl_sem.h"

#define SEM_R	0400
#define SEM_W	0200

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

int rtl_sem_init(int n)
{
	return semget(IPC_PRIVATE, n, (SEM_R | SEM_W | IPC_CREAT));
}

/* init semaphore by semctl */
int rtl_sem_set(int id, int value)
{
	union semun sem_union;

	sem_union.val = value;
	if (semctl(id, 0, SETVAL, sem_union) == -1)
		return -1;

	return 0;
}

/* delete semaphore by sectl */
int rtl_sem_del(int id)
{
	union semun sem_union;

	if (semctl(id, 0, IPC_RMID, sem_union) == -1)
		return -1;

	return 0;
}

/* P(v) */
int rtl_sem_p(int id)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;	/* P(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(id, &sem_b, 1) == -1)
		return -1;

	return 0;
}

/* V(v) */
int rtl_sem_v(int id)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;	/* V(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(id, &sem_b, 1) == -1)
		return -1;

	return 0;
}
