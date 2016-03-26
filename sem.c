#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>

#include "sem.h"

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

/* init semaphore by semctl */
int set_semvalue(int sem_id, int value)
{
	union semun sem_union;

	sem_union.val = value;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
	#if DEBUG
		fprintf(stderr, "set semaphore failed\n");
	#endif
		return -1;
	}

	return 0;
}


/* delete semaphore by sectl */
int del_semvalue(int sem_id)
{
	union semun sem_union;

	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1) {
	#if DEBUG
		fprintf(stderr, "delete semaphore failed\n");
	#endif
		return -1;
	}
	return 0;
}

/* p(v) */
int sem_p(int sem_id)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;	/* P(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_b, 1) == -1) {
	#if DEBUG
		fprintf(stderr, "semaphore_p failed\n");
		return -1;
	#endif
	}

	return 0;
}

/* v(v) */
int sem_v(int sem_id)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;	/* V(v) */
	sem_b.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_b, 1) == -1) {
	#if DEBUG
		fprintf(stderr, "semaphore_v failed\n");
		return -1;
	#endif
	}

	return 0;
}
