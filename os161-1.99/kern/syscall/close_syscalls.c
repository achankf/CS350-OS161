#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <proc.h>
#include <current.h>

int sys_close(int fd);
{

	struct vnode *vn;
	struct fd_tuple * fd_t = fdtable[fd];

	vn = fd_t->vn;

	vfs_close(vn)

	// recycle fd back to fdtable
	int ctr = --(fd_t->counter);

	// no more references
	if (ctr == 0) {
		// free fd_tuple
		lock_destroy(fd_t->lock);
		kfree(fd_t);
	}

	idgen_put_back(curproc->fd_idgen, fd);

	return 0;

}

