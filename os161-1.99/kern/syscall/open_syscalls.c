#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <limits.h>

int sys_open(const char *filename, int flags, int mode);
{
	struct vnode *vn = NULL;

	// check params
	if (filename == NULL) {
		// return
	}

	char * mut_filename = NULL;
	mut_filename = kstrdup(filename);

	// vfs_open(char *path, int openflags, mode_t mode, struct vnode **ret)
	int result = vfs_open(filename, flags, mode, &vn)
	kfree(mut_filename);

	// search if vnode already exists
	for (int i=0; i<OPEN_MAX; i++) {
		if (vn == fdtable[i]->vn) {
			return i;
		}
	}

	int fd = idgen_get_next(curproc->fd_idgen);

	// initialize fd_tuple and alloc mem if necessary
	struct fd_tuple *node = kmalloc(sizeof(fd_tuple));
	node->vn = *vn;
	node->offset = 0;
	node->counter = 1;
	node->lock = lock_create("lock");

	curproc->fdtable[fd]=node;

	return fd;

}

