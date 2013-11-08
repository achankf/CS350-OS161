#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <addrspace.h>
#include <current.h>
#include <vnode.h>
#include <thread.h>
#include <uio.h>
#include <proc.h>
#include <kern/errno.h>
#include <kern/fcntl.h>

int sys_read(int fd, void *buf, size_t buflen, int *retval) {
	int result;
	struct fd_tuple *tuple;
	struct iovec iov;
	struct uio ku;

	result = check_valid_userptr((const_userptr_t)buf);
	if (result) return result;

	spinlock_acquire(&curproc->p_lock);
		if (!proc_valid_fd(curproc, fd)){
			spinlock_release(&curproc->p_lock);
			return EBADF;
		}
		tuple = curproc->fdtable[fd];
	spinlock_release(&curproc->p_lock);

	lock_acquire(tuple->lock);

	const int how = tuple->flags & O_ACCMODE;
	if (how == O_WRONLY){
		lock_release(tuple->lock);
		return EBADF;
	}

	uio_kinit(&iov,&ku, buf, buflen,tuple->offset,UIO_READ);
	result = VOP_READ(tuple->vn, &ku);
    
	if (result != 0) {
		// need to change errno	    
		lock_release(tuple->lock);
		return result;
	}

	tuple->offset = ku.uio_offset;

	lock_release(tuple->lock);
	*retval = buflen - ku.uio_resid;
	return 0;
}

