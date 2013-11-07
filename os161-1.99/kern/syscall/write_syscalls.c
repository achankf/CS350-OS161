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
#include <synch.h>

// so far just changed somethings from sys_read
// might not work

int sys_write(int fd, userptr_t user_buffer, size_t numBytes) {
	if (fd == 1) {
        kprintf((char *) user_buffer);
        return numBytes;
    }

    int result;
	struct fd_tuple *fd_t = curproc->fdtable[fd];

    lock_acquire(fd_t->lock);

	struct iovec iov;
	struct uio ku;

    uio_kinit(&iov,&ku, user_buffer, numBytes,fd_t->offset,UIO_WRITE);

	result = VOP_WRITE(fd_t->vn, &ku);
	if (result!= 0) {
		// need to change errno	    
	    return -1;
	}

	fd_t->offset = ku.uio_offset;

	if(ku.uio_resid > 0) {
		return numBytes;
	} else {
		return 0;
	}
    
}

