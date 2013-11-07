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

// so far just changed somethings from sys_read
// might not work

int sys_write(int fd, void *buf, size_t buflen, int *retval) {
	int result;
	struct fd_tuple *tuple;
	struct iovec iov;
	struct uio ku;

	spinlock_acquire(&curproc->p_lock);
		tuple = curproc->fdtable[fd];
	spinlock_release(&curproc->p_lock);

	lock_acquire(tuple->lock);

	uio_kinit(&iov,&ku, buf, buflen,tuple->offset,UIO_WRITE);
	result = VOP_WRITE(tuple->vn, &ku);
    
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

