#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <kern/errno.h>
#include <current.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <synch.h>
#include <limits.h>

int sys_fork(struct trapframe *tf, pid_t *ret){
	struct proc * temp;

	KASSERT(curproc->pid > 0);

	if (proc_reach_limit()){
		return ENPROC;
	}

	// p_name gets deep-copied by proc_create
	temp = proc_create_runprogram(curproc->p_name);
	if (temp == NULL) return ENOMEM;

	DEBUG(DB_EXEC, "FORKING PROCESS %d into %d\n", curproc->pid, temp->pid);

	// copy the address space
	as_copy(curproc->p_addrspace, &temp->p_addrspace);
	if (temp->p_addrspace == NULL){
		proc_destroy(temp);
		return ENOMEM;
	}

	temp->parent = curproc->pid;
	*ret = temp->pid;

	for (int i = 0; i < OPEN_MAX; i++){
		temp->fdtable[i] = curproc->fdtable[i];
	}

	struct semaphore *fork_sem = sem_create("fork_sem", 0);
	DEBUG(DB_EXEC, "fork_sem: I will make sure tf is copied by child thread before *my* process stack gets deleted.\n");

	thread_fork("make child", temp, enter_forked_process, (void*)tf, (unsigned long) fork_sem);

	DEBUG(DB_EXEC, "fork: waiting trapframe to be copied\n");
	P(fork_sem);
	sem_destroy(fork_sem);

	return 0;
}
