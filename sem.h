#ifndef _SEM_H_
#define _SEM_H_

#define SEM_SEED	1000

int set_semvalue(int sem_id, int value);
int del_semvalue(int sem_id);
int sem_p(int sem_id);
int sem_v(int sem_id);

#endif /* _SEM_H_ */
