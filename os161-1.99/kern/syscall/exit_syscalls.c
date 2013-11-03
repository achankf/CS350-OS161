#include <types.h>
#include <syscall.h>
#include <vnode.h>
#include <lib.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>

int sys__exit(int exit_code)
{
	DEBUG(DB_EXEC, "Exiting %d\n", sys_getpid());

	// destroy the address space
	if (curproc->p_addrspace) {
		struct addrspace *as;

		as_deactivate();
		as = curproc_setas(NULL);
		as_destroy(as);
	}

	/* VFS fields */
	if (curproc->p_cwd) {
		VOP_DECREF(curproc->p_cwd);
		curproc->p_cwd = NULL;
	}

	// clean up unneeded memory
	spinlock_cleanup(&curproc->p_lock);
	idgen_destroy(curproc->fd_idgen);
	cv_destroy(curproc->waitfor_child);
	kfree(curproc->p_name);

	if (curproc->parent <= 1){
		// not parent to clean up its mess -- suicide and become a zombie directly
		proc_destroy(curproc);
		thread_force_zombie();
	} else {
		// parent do the dirty work of destroying a thread
		proc_i_died(exit_code);
		thread_exit();
	}

	panic("sys__exit: thread_exit should not return");
	return -1;
}
