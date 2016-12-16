#ifndef _RTL_SEM_H_
#define _RTL_SEM_H_

int rtl_sem_init(int n);
int rtl_sem_set(int id, int value);
int rtl_sem_del(int id);
int rtl_sem_p(int id);
int rtl_sem_v(int id);

#endif /* _RTL_SEM_H_ */
