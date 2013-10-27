#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>

int sys_waitpid(pid_t pid, int *status, int option){

kprintf("[kernel]: Waiting for PID %d\n",pid);

	if (option != 0){
		return EINVAL;
	}
kprintf("Test 1: %d\n",!proc_exists(pid));
	if (!proc_exists(pid)){
		return ESRCH;
	}
kprintf("Test 2: %d %d\n",proc_get_parent(pid), sys_getpid());
	if (proc_get_parent(pid) != sys_getpid()){
		return ECHILD;
	}
kprintf("Test 3: %d for status pointer %p\n",!VALID_USERPTR(status), status);
kprintf("%d %d\n", VALID_PTR(status), VALID_USERPTR(status));
	if (!VALID_USERPTR(status)){
		return EFAULT;
	}
kprintf("All precondition okay\n");

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
		proctable_unlink(pid);
	lock_release(curproc->waitlock);

	*status = pid;
kprintf("Done waiting\n");

	return 0;
}
