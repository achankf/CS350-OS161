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

// not tested

int sys_read(int fd, void *buf, size_t buflen, int *retval) {
	int result;
	struct fd_tuple *fd_t = curproc->fdtable[fd];
    
	struct iovec iov;
	struct uio ku;

	lock_acquire(fd_t->lock);

	uio_kinit(&iov,&ku, buf, buflen,fd_t->offset,UIO_READ);
	result = VOP_READ(fd_t->vn, &ku);
    
	if (result != 0) {
		// need to change errno	    
		lock_release(fd_t->lock);
		return -1;
	}

	fd_t->offset = ku.uio_offset;

	lock_release(fd_t->lock);
	*retval = buflen;
	return 0;
}

