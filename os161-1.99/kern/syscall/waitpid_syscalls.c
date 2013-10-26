#include <types.h>
#include <syscall.h>

int sys_waitpid(pid_t pid, int *status, int option){
	(void)pid;
	(void)status;
	(void)option;
	return -1;
}
