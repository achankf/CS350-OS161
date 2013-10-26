#include <types.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>

pid_t sys_getpid(void){
	KASSERT(curproc != NULL);
	return curproc->pid;
}
