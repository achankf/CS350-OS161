#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <addrspace.h>
#include <vnode.h>
#include <thread.h>
#include <uio.h>
#include <proc.h>

// no tested

int sys_read(int fd, void *buf, size_t buflen);
{
	int result;
	struct fd_tuple *fd_t = fdtable[fd];

	struct iovec iov;
	struct uio u;

	iov.iov_ubase = buf;
	iov.iov_len = buflen;		 // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;		// amount to read from the file
	u.uio_offset = fd_t->offset;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curproc_getas();

	result = VOP_READ(fd_t->vn, &u));
	if (result!= 0) {
		// need to change errno	    

	    return -1;

	}

	fd_t->offset = u.uio_offset;

	if(u.uio_resid > 0) {
		return buflen;
	} else {
		return 0;
	}
	
}

