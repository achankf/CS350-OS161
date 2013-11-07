#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <limits.h>
#include <kern/errno.h>

int sys_open(const char *filename, int flags, int *retval)
{

	int result, fd;
	struct fd_tuple *tuple;

	if (filename == NULL) {
		return EFAULT;
	} 

	if (proc_file_reach_limit(curproc)){
		return EMFILE;
	}

	result = fd_tuple_create(filename, flags, 0, &tuple);
	if (result){
		return result;
	}

	fd = idgen_get_next(curproc->fd_idgen);

	curproc->fdtable[fd]=tuple;
	*retval = fd;

	return 0;

}

