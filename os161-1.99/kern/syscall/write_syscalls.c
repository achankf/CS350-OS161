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

int sys_write(int fd, userptr_t user_buffer, size_t numBytes) {

	int result;
	struct fd_tuple *fd_t = curproc->fdtable[fd];

	struct iovec iov;
	struct uio u;

	iov.iov_ubase = user_buffer;
	iov.iov_len = numBytes;		 // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = numBytes;		// amount to read from the file
	u.uio_offset = fd_t->offset;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = curproc_getas();

	result = VOP_WRITE(fd_t->vn, &u);
	if (result!= 0) {
		// need to change errno	    

	    return -1;

	}

	fd_t->offset = u.uio_offset;

	if(u.uio_resid > 0) {
		return numBytes;
	} else {
		return 0;
	}
	
}

