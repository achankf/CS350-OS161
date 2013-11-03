#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <kern/errno.h>
#include <current.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <synch.h>

int sys_fork(struct trapframe *tf, pid_t *ret){
	struct proc * temp;

	KASSERT(curproc->pid > 0);

	if (proc_reach_limit()){
		return ENPROC;
	}

	// p_name gets deep-copied by proc_create
	temp = proc_create_runprogram(curproc->p_name);
	if (temp == NULL) return ENOMEM;

	DEBUG(DB_EXEC, "FORKED %d\n", temp->pid);

	struct trapframe *newp_tf = kmalloc(sizeof(struct trapframe));
	if (newp_tf == NULL){
		proc_destroy(temp);
		return ENOMEM;
	}

	// copy the address space
	as_copy(curproc->p_addrspace, &temp->p_addrspace);
	if (temp->p_addrspace == NULL){
		kfree(newp_tf);
		proc_destroy(temp);
		return ENOMEM;
	}

	temp->parent = curproc->pid;
	*ret = temp->pid;

	for (int i = 0; i < FDTABLE_SIZE; i++){
		temp->fdtable[i] = curproc->fdtable[i];
	}

	// structure of integral values => deep copy of the trap frame
	*newp_tf = *tf;
	thread_fork("", temp, enter_forked_process, (void*)newp_tf, 0);

	return 0;
}
