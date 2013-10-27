#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <kern/errno.h>
#include <current.h>
#include <addrspace.h>

int sys_fork(pid_t *ret){
	struct proc * temp;

	// p_name gets deep-copied by proc_create
	temp = proc_create_runprogram(curproc->p_name);
	if (temp == NULL) return ENOMEM;

	// copy the address space
	as_copy(curproc->p_addrspace, &temp->p_addrspace);
	if (temp->p_addrspace == NULL){
		proc_destroy(temp);
		return ENOMEM;
	}

	temp->parent = curproc->pid;

	*ret = temp->pid;
	return 0;
}
