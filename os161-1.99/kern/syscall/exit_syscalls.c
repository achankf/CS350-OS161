#include <types.h>
#include <syscall.h>
#include <vnode.h>
#include <lib.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>

void sys__exit(int exit_code)
{
	DEBUG(DB_EXEC, "Exiting %d\n", sys_getpid());

	proc_destroy_addrspace(curproc);

	/* VFS fields */
	if (curproc->p_cwd) {
		VOP_DECREF(curproc->p_cwd);
		curproc->p_cwd = NULL;
	}

	// clean up unneeded memory
	idgen_destroy(curproc->fd_idgen);
	cv_destroy(curproc->waitfor_child);
	kfree(curproc->p_name);

	struct lock *proctable_lock = proctable_lock_get();

	DEBUG(DB_EXEC,"proc_i_died: pid:%d exit:%d\n", sys_getpid(), exit_code);

	// make sure either children die or parent dies -- not both
	lock_acquire(proctable_lock);

	struct proc *parent = proc_get_parent(curproc);

	// not parent to clean up its mess -- suicide and become a zombie directly
	if (parent != NULL && curproc->parent != 1){
		// ask parent to kill me
		curproc->zombie = true;
		curproc->retval = exit_code;
		cv_signal(parent->waitfor_child, proctable_lock);
		lock_release(proctable_lock);
		thread_exit();
		panic("Exited thread not allowed to come up!");
	} else {
		proc_destroy(curproc);
		lock_release(proctable_lock);
		thread_force_zombie();
		panic("The zombie walks!");
	}
	// not reached
}
