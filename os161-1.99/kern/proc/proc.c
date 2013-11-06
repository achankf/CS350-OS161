/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Process support.
 *
 * There is (intentionally) not much here; you will need to add stuff
 * and maybe change around what's already present.
 *
 * p_lock is intended to be held when manipulating the pointers in the
 * proc structure, not while doing any significant work with the
 * things they point to. Rearrange this (and/or change it to be a
 * regular lock) as needed.
 *
 * Unless you're implementing multithreaded user processes, the only
 * process that will have more than one thread is the kernel process.
 */

#include <types.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <array.h>
#include <id_generator.h>
#include <syscall.h>

#define PTABLE_SIZE 512

/*
 * The process for the kernel; this holds all the kernel-only threads.
 */
struct proc *kproc;

struct id_generator *pidgen;
struct proc *proctable[PTABLE_SIZE];
struct lock *proctable_lock;


/*
 * Create a proc structure.
 */
static
struct proc *
proc_create(const char *name)
{
	struct proc *proc;

	proc = kmalloc(sizeof(*proc));
	if (proc == NULL) {
		return NULL;
	}

	proc->waitfor_child = cv_create("");
	if (proc->waitfor_child == NULL){
		kfree(proc);
		return NULL;
	}
	proc->p_name = kstrdup(name);
	if (proc->p_name == NULL) {
		cv_destroy(proc->waitfor_child);
		kfree(proc);
		return NULL;
	}

	proc->fd_idgen = idgen_create(3);
	if (proc->fd_idgen == NULL){
		cv_destroy(proc->waitfor_child);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}

	proc->children = q_create(10);

	proc->pid = idgen_get_next(pidgen);
	if (proc->pid >= PTABLE_SIZE){ // no need to recover pid
		cv_destroy(proc->waitfor_child);
		kfree(proc->fd_idgen);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}

	threadarray_init(&proc->p_threads);
	spinlock_init(&proc->p_lock);

	/* VM fields */
	proc->p_addrspace = NULL;

	/* VFS fields */
	proc->p_cwd = NULL;

	bzero(proc->fdtable, sizeof(struct fd_tuple*) * OPEN_MAX);

	proctable[proc->pid] = proc;

	return proc;
}

/*
 * Destroy a proc structure.
 *
 * Note: nothing currently calls this. Your wait/exit code will
 * probably want to do so.
 */
void
proc_destroy(struct proc *proc)
{
	/*
	 * You probably want to destroy and null out much of the
	 * process (particularly the address space) at exit time if
	 * your wait/exit design calls for the process structure to
	 * hang around beyond process exit. Some wait/exit designs
	 * do, some don't.
	 */

	DEBUG(DB_EXEC,"Destroying process %d\n", proc->pid);

	KASSERT(proc != NULL);
	KASSERT(proc != kproc);
	KASSERT(lock_do_i_hold(proctable_lock));

	// clean link on the proctable
	proctable[proc->pid] = NULL;

	if (proc->children == NULL) goto SKIP_KILLING_CHILDREN;

	while (!q_empty(proc->children)){
		pid_t child = (pid_t) q_remhead(proc->children);
		if (!proctable[child]->zombie) continue;
		proc_destroy(proctable[child]);
	}

	q_destroy(proc->children);

SKIP_KILLING_CHILDREN:

	// recover the unused pid
	idgen_put_back(pidgen, proc->pid);

	// remove process reference from its associated threads
	int size = threadarray_num(&proc->p_threads);
	for (int i = 0; i < size; i++){
		struct thread *temp = threadarray_get(&proc->p_threads, i);
		proc_remthread(temp);
	}

	// most memory references are cleaned by sys__exit

	spinlock_cleanup(&proc->p_lock);
	threadarray_cleanup(&proc->p_threads);
	kfree(proc);
}

/*
 * Create the process structure for the kernel.
 */
void
proc_bootstrap(void)
{
	pidgen = idgen_create(1);
	proctable_lock = lock_create("proctable lock");
	bzero(proctable, sizeof(struct proc*) * PTABLE_SIZE);
	

	kproc = proc_create("[kernel]");
	if (kproc == NULL) {
		panic("proc_create for kproc failed\n");
	}
	#if OPT_A1
		kprintf("kproc = %p\n", kproc);
	#endif
}

/*
 * Create a fresh proc for use by runprogram.
 *
 * It will have no address space and will inherit the current
 * process's (that is, the kernel menu's) current directory.
 */
struct proc *
proc_create_runprogram(const char *name)
{
	struct proc *proc;

	proc = proc_create(name);
	if (proc == NULL) {
		return NULL;
	}

	/* VM fields */

	proc->p_addrspace = NULL;

	/* VFS fields */

	spinlock_acquire(&curproc->p_lock);
	/* we don't need to lock proc->p_lock as we have the only reference */
	if (curproc->p_cwd != NULL) {
		VOP_INCREF(curproc->p_cwd);
		proc->p_cwd = curproc->p_cwd;
	}
	spinlock_release(&curproc->p_lock);

	return proc;
}

/*
 * Add a thread to a process. Either the thread or the process might
 * or might not be current.
 */
int
proc_addthread(struct proc *proc, struct thread *t)
{
	int result;

	KASSERT(t->t_proc == NULL);

	spinlock_acquire(&proc->p_lock);
	result = threadarray_add(&proc->p_threads, t, NULL);
	spinlock_release(&proc->p_lock);
	if (result) {
		return result;
	}
	t->t_proc = proc;
	return 0;
}

/*
 * Remove a thread from its process. Either the thread or the process
 * might or might not be current.
 */
void
proc_remthread(struct thread *t)
{
	struct proc *proc;
	unsigned i, num;

	proc = t->t_proc;
	KASSERT(proc != NULL);

	spinlock_acquire(&proc->p_lock);
	/* ugh: find the thread in the array */
	num = threadarray_num(&proc->p_threads);
	for (i=0; i<num; i++) {
		if (threadarray_get(&proc->p_threads, i) == t) {
			threadarray_remove(&proc->p_threads, i);
			spinlock_release(&proc->p_lock);
			t->t_proc = NULL;
			return;
		}
	}
	/* Did not find it. */
	spinlock_release(&proc->p_lock);
	panic("Thread (%p) has escaped from its process (%p)\n", t, proc);
}

/*
 * Fetch the address space of the current process. Caution: it isn't
 * refcounted. If you implement multithreaded processes, make sure to
 * set up a refcount scheme or some other method to make this safe.
 */
struct addrspace *
curproc_getas(void)
{
	struct addrspace *as;
#ifdef UW
        /* Until user processes are created, threads used in testing 
         * (i.e., kernel threads) have no process or address space.
         */
	if (curproc == NULL) {
		return NULL;
	}
#endif

	// already has a lock
	if (spinlock_do_i_hold(&curproc->p_lock)){
		as = curproc->p_addrspace;
	} else { // no lock
		spinlock_acquire(&curproc->p_lock);
			as = curproc->p_addrspace;
		spinlock_release(&curproc->p_lock);
	}
	return as;
}

static
struct addrspace *
proc_setas(struct proc *proc, struct addrspace *newas){
	struct addrspace *oldas;

	spinlock_acquire(&proc->p_lock);
	oldas = proc->p_addrspace;
	proc->p_addrspace = newas;
	spinlock_release(&proc->p_lock);
	return oldas;
}

/*
 * Change the address space of the current process, and return the old
 * one.
 */
struct addrspace *
curproc_setas(struct addrspace *newas){
	return proc_setas(curproc, newas);
}

inline static bool
proc_within_bound(pid_t pid){
	return pid >= 0 && pid < PTABLE_SIZE;
}

bool
proc_exists(pid_t pid){
	return proc_within_bound(pid) && proctable[pid] != NULL;
}

struct proc *proc_getby_pid(pid_t pid){
	KASSERT(proc_exists(pid));
	return proctable[pid];
}

struct proc *proc_get_parent(struct proc *proc){
	KASSERT(proc != NULL);
	return proctable[proc->parent];
}

bool proc_reach_limit(){
	return idgen_reach(pidgen, PTABLE_SIZE);
}

struct lock *proctable_lock_get(){
	return proctable_lock;
}

void proc_destroy_addrspace(struct proc *proc){
	if (proc->p_addrspace) {
		as_destroy(proc_setas(proc,NULL));
	}
}
