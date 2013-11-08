#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <kern/errno.h>

int sys_close(int fd)
{

	spinlock_acquire(&curproc->p_lock);

		if (!proc_valid_fd(curproc, fd)){
			spinlock_release(&curproc->p_lock);
			return EBADF;
		}

		fd_tuple_give_up(curproc->fdtable[fd]);
		curproc->fdtable[fd] = NULL;
	spinlock_release(&curproc->p_lock);
	idgen_put_back(curproc->fd_idgen, fd);

	return 0;

}

