#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "rtl_proc.h"
#include "rtl_log.h"

int rtl_last_proc;
int rtl_proc_slot;

rtl_proc_t rtl_processes[RTL_MAX_PROCESSES];

int rtl_proc_spawn(rtl_spawn_proc_pt proc, void *args, char *name, int respawn)
{
	int s;
	pid_t pid;
	rtl_spawn_proc_pt proc_tmp = proc;
	void *args_tmp = args;

	for (s = 0; s < rtl_last_proc; s++) {
		if (rtl_processes[s].pid == -1) {	/* must exited */
			proc_tmp = rtl_processes[s].proc;
			args_tmp = rtl_processes[s].args;
			break;
		}
	}

	if (s == RTL_MAX_PROCESSES) {
		rtl_log_write(RTL_LOG_ERR, "no more than %d rtl_processes can be spawned",
					  RTL_MAX_PROCESSES);
		return -1;
	}

	rtl_proc_slot = s;

	pid = fork();
	if (pid < 0) {
		return -1;
	} else if (pid == 0) {
		proc_tmp(args_tmp);
	}

	rtl_processes[s].pid = pid;

	if (respawn)
		return pid;

	rtl_processes[s].args = args;
	rtl_processes[s].name = name;
	rtl_processes[s].proc = proc;

	if (s == rtl_last_proc)
		rtl_last_proc++;

	return pid;
}

void rtl_proc_wait(void)
{
	pid_t pid;
	int i, status;
	char *proc_name;

	for (;;) {
		pid = waitpid(-1, &status, 0);
		if (pid == -1) {
			rtl_log_write(RTL_LOG_ERR, "waitpid error: %s", strerror(errno));
			continue;
		}

		proc_name = "unknown process";

		for (i = 0; i < rtl_last_proc; i++) {
			if (rtl_processes[i].pid == pid) {
				rtl_processes[i].pid = -1;
				/* respawn process */
				rtl_proc_spawn(NULL, NULL, NULL, 1);
				proc_name = rtl_processes[i].name;
				break;
			}
		}

		if (WTERMSIG(status)) {
			rtl_log_write(RTL_LOG_INFO, "%s(%d) exited on signal %d", proc_name, pid,
						  WTERMSIG(status));
		} else {
			rtl_log_write(RTL_LOG_INFO, "%s(%d) exited with code %d", proc_name, pid,
						  WEXITSTATUS(status));
		}
	}
}
