#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <kern/errno.h>
#include <current.h>
#include <spl.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <synch.h>

int sys_fork(struct trapframe *tf, pid_t *ret){
	struct proc * temp;
	int result;
	int spl;

	KASSERT(curproc->pid > 0);

	spl = splhigh();

	if (proc_reach_limit()){
		splx(spl);
		return ENPROC;
	}

	// p_name gets deep-copied by proc_create
	temp = proc_create_runprogram(curproc->p_name);
	if (temp == NULL){
		splx(spl);
		return ENOMEM;
	}

	DEBUG(DB_EXEC, "FORKING PROCESS %d into %d\n", curproc->pid, temp->pid);

	// copy the address space
	as_copy(curproc->p_addrspace, &temp->p_addrspace);
	if (temp->p_addrspace == NULL){
		splx(spl);
		lock_acquire(proctable_lock_get());
			proc_destroy(temp);
		lock_release(proctable_lock_get());
		return ENOMEM;
	}

	DEBUG(DB_EXEC, "\tfork: address space created");

	lock_acquire(proctable_lock_get());

	result = q_addtail(curproc->children, (void*)temp->pid);
	if (result != 0) {
		splx(spl);
		proc_destroy_addrspace(temp);
		proc_destroy(temp);
		lock_release(proctable_lock_get());
		return ENOMEM;
	}

	for (int i = 0; i < FDTABLE_SIZE; i++){
		temp->fdtable[i] = curproc->fdtable[i];
	}

	for (int i = 0; i < FDTABLE_SIZE; i++){
		temp->fdtable[i] = curproc->fdtable[i];
	}

	temp->parent = curproc->pid;

	*ret = temp->pid;

	struct semaphore *fork_sem = sem_create("fork_sem", 0);
	if (fork_sem == NULL){
		splx(spl);
		proc_destroy_addrspace(temp);
		proc_destroy(temp);
		lock_release(proctable_lock_get());
		return ENOMEM;
	}
	

	DEBUG(DB_EXEC, "fork_sem: I will make sure tf is copied by child thread before *my* process stack gets deleted.\n");

	result = thread_fork("make child", temp, enter_forked_process, (void*)tf, (unsigned long) fork_sem);

	DEBUG(DB_EXEC, "fork: waiting trapframe to be copied\n");

	if (result){
		splx(spl);
		proc_destroy_addrspace(temp);
		proc_destroy(temp);
		lock_release(proctable_lock_get());
		sem_destroy(fork_sem);
		return ENOMEM;
	} else {
		lock_release(proctable_lock_get());
		splx(spl);
		P(fork_sem);
	}

	sem_destroy(fork_sem);

	*ret = temp->pid;
	return 0;
}
