#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>
#include <copyinout.h>

int sys_waitpid(pid_t pid, int *status, int option){
	struct proc *child;
	int result;
	char buf[1]; // dummy that is used to check for invalid status ptr

	DEBUG(DB_EXEC, "---------------- waitpid of %d on %d begins ----------------\n",sys_getpid(),pid);

	if (option != 0){
		return EINVAL;
	}

	DEBUG(DB_EXEC, "Test: %d for status pointer %p\n",!VALID_USERPTR(status), status);
	if (status == NULL || !VALID_USERPTR(status)){
		return EFAULT;
	}

	result = copyin((const_userptr_t)status, buf, 1);
	if (result){
		return result;
	}

	lock_acquire(proctable_lock_get());

		DEBUG(DB_EXEC, "Test: %d\n",!proc_exists(pid));
		if (!proc_exists(pid)){
			lock_release(proctable_lock_get());
			return ESRCH;
		}

		DEBUG(DB_EXEC,"Test: %d %d\n",proc_getby_pid(pid)->parent, curproc->pid);
		if (curproc->pid != proc_getby_pid(pid)->parent){
			lock_release(proctable_lock_get());
			return ECHILD;
		}

		DEBUG(DB_EXEC,"All precondition okay\n");

		child = proc_getby_pid(pid);
		while(!child->zombie){
			cv_wait(curproc->waitfor_child, proctable_lock_get());
		}
		*status = child->retval;
	lock_release(proctable_lock_get());

	DEBUG(DB_EXEC, "---------------- waitpid of %d on %d ends ----------------\n", sys_getpid(), pid);

	return 0;
}
