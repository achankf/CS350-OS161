#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <proc.h>


int sys_read(int fd, void *buf, size_t buflen);
{
	// curproc->fdtable[fd].status = 0;
	// curproc->fdtable

	int result;
	size_t stoplen;

	result = copycheck(usersrc, len, &stoplen);
	if (result) {
		return result;
	}
	if (stoplen != len) {
		/* Single block, can't legally truncate it. */
		return EFAULT;
	}

	// memcpy (dest, src, len);
	memcpy(buf, curproc->fdtable[fd]->vn->vn_data, buflen);


	return 0;

}

