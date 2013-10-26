#include <types.h>
#include <kern/errno.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>

int sys_waitpid(pid_t pid, int *status, int option){

	if (option != 0){
		return EINVAL;
	}
	if (!proc_exists(pid)){
		return ESRCH;
	}
	if (proc_get_parent(pid) != sys_getpid()){
		return ECHILD;
	}
	if (status == NULL){
		return EFAULT;
	}

	lock_acquire(curproc->waitlock);
		int len = q_len(curproc->zombie_children);

		// keep looking for zombie children util one of them is the target
		while (true){
			for (int i = 0; i < len; i++){
				pid_t child = (pid_t)q_remhead(curproc->zombie_children);

				// child found
				if (child == pid) goto FOUND;

				// put back the child if not the target
				q_addtail(curproc->zombie_children, (void*)child);
			}
			cv_wait(curproc->waitfor_child, curproc->waitlock);
		}
FOUND:
	lock_release(curproc->waitlock);

	*status = pid;

	return 0; // return pid in syscall.c
}
