#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <thread.h>
#include <uio.h>
#include <proc.h>
#include <kern/errno.h>
#include <kern/fcntl.h>

// so far just changed somethings from sys_read
// might not work

int sys_write(int fd, void *buf, size_t buflen, int *retval) {
	int result;
	struct fd_tuple *tuple;
	struct iovec iov;
	struct uio ku;

	if (!proc_valid_fd(curproc, fd)) return EBADF;
	if (!VALID_USERPTR(buf)) return EFAULT;

	spinlock_acquire(&curproc->p_lock);
		tuple = curproc->fdtable[fd];
	spinlock_release(&curproc->p_lock);

	lock_acquire(tuple->lock);

	const int how = tuple->flags & O_ACCMODE;
	if (how == O_RDONLY) return EBADF;

	uio_kinit(&iov,&ku, buf, buflen,tuple->offset,UIO_WRITE);
	result = VOP_WRITE(tuple->vn, &ku);
    
	if (result != 0) {
		lock_release(tuple->lock);
		return result;
	}

	tuple->offset = ku.uio_offset;

	lock_release(tuple->lock);
	*retval = buflen - ku.uio_resid;
	return 0;
}

