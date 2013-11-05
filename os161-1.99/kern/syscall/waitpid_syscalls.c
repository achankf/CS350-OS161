#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>

int sys_waitpid(pid_t pid, int *status, int option){
	struct proc *child;

	DEBUG(DB_EXEC, "---------------- waitpid of %d on %d begins ----------------\n",sys_getpid(),pid);

	if (option != 0){
		return EINVAL;
	}
	DEBUG(DB_EXEC, "Test 1: %d\n",!proc_exists(pid));
	if (!proc_exists(pid)){
		return ESRCH;
	}
	DEBUG(DB_EXEC,"Test 2: %d %d\n",proc_getby_pid(pid)->parent, curproc->pid);
	if (curproc->pid != proc_getby_pid(pid)->parent){
		return ECHILD;
	}
	DEBUG(DB_EXEC, "Test 3: %d for status pointer %p\n",!VALID_USERPTR(status), status);
	if (!VALID_USERPTR(status)){
		return EFAULT;
	}
	DEBUG(DB_EXEC,"All precondition okay\n");

	lock_acquire(proctable_lock_get());
		child = proc_getby_pid(pid);
		while(!child->zombie){
			cv_wait(curproc->waitfor_child, proctable_lock_get());
		}
		*status = child->retval;
	lock_release(proctable_lock_get());

	DEBUG(DB_EXEC, "---------------- waitpid of %d on %d ends ----------------\n", sys_getpid(), pid);

	return 0;
}
