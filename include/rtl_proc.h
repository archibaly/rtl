#ifndef _RTL_PROC_H_
#define _RTL_PROC_H_

#include <unistd.h>

#define RTL_MAX_PROCESSES	128

typedef void (*rtl_spawn_proc_pt) (void *args);

typedef struct {
	pid_t pid;
	void *args;
	char *name;
	int status;
	rtl_spawn_proc_pt proc;
} rtl_proc_t;

int rtl_proc_spawn(rtl_spawn_proc_pt proc, void *args, char *name, int respawn);
void proc_wait(void);

extern int rtl_last_proc;
extern int rtl_proc_slot;
extern rtl_proc_t rtl_processes[];

#endif /* _RTL_PROC_H_ */
