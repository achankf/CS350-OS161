#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <current.h>

int sys_open(const char *filename, int flags, int mode);
{
	struct vnode ** vn;

	// vfs_open(char *path, int openflags, mode_t mode, struct vnode **ret)
	vfs_open(filename, flags, mode,vn)

	int result = idgen_get_next(curproc->fd_idgen);

	struct fd_tuple *node;
	node->vn = vn;

	curproc->fdtable[result]=node;

	return result;

}

