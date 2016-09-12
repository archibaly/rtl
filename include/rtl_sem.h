#ifndef _RTL_SEM_H_
#define _RTL_SEM_H_

#define RTL_SEM_SEED	1000

int rtl_set_semvalue(int sem_id, int value);
int rtl_del_semvalue(int sem_id);
int rtl_sem_p(int sem_id);
int rtl_sem_v(int sem_id);

#endif /* _RTL_SEM_H_ */
