#include <fd_tuple.h>
#include <vfs.h>
#include <vnode.h>
#include <lib.h>
#include <synch.h>
#include <kern/fcntl.h>
#include <kern/errno.h>

struct fd_tuple *stdinput, *stdoutput;

int fd_tuple_create(const char *filename, int flags, mode_t mode, struct fd_tuple **rettuple){

	struct vnode *vn;
	struct fd_tuple *node = kmalloc(sizeof(struct fd_tuple));

	if (node == NULL) return ENOMEM;
	if (flags & O_APPEND) return EINVAL;

	node->lock = lock_create("");

	if (node->lock == NULL){
		kfree(node);
		return ENOMEM;
	}

	char *filenamecopy = kstrdup(filename);
	if (filenamecopy == NULL){
		lock_destroy(node->lock);
		kfree(node);
		return ENOMEM;
	}

	int result = vfs_open(filenamecopy, flags, mode, &vn);
	kfree(filenamecopy);

	if (result){
		lock_destroy(node->lock);
		kfree(node);
		return result;
	}

	node->vn = vn;
	node->offset = 0;
	node->counter = 1;
	node->flags = flags;

	*rettuple = node;

	return 0;
}

static void fd_tuple_destroy(struct fd_tuple *tuple){
	vfs_close(tuple->vn);
	lock_destroy(tuple->lock);
	kfree(tuple);
}

void fd_tuple_give_up(struct fd_tuple *tuple){

	/* if the tuple is NULL or it points to the global
	 * tuples (i.e. stdin, stdout), then return immediately
	 */
	if (tuple == NULL
		|| tuple == fd_tuple_stdin()
		|| tuple == fd_tuple_stdout()) return;

	lock_acquire(tuple->lock);

	DEBUG(DB_EXEC, "fd_tuple give_up: count left:%d\n", tuple->counter);
		KASSERT(tuple->counter > 0);
		tuple->counter--;

		if (tuple->counter == 0){ // need to be check within critical section
			lock_release(tuple->lock);
			fd_tuple_destroy(tuple);
		} else {
			lock_release(tuple->lock);
		}
}

void fd_tuple_bootstrap(){
	int result;

	// mode is not used
	result = fd_tuple_create("con:", O_RDONLY, 0, &stdinput);
	KASSERT(result == 0);
	result = fd_tuple_create("con:", O_WRONLY, 0, &stdoutput);
	KASSERT(result == 0);
}

struct fd_tuple *fd_tuple_stdin(){
	return stdinput;
}

struct fd_tuple *fd_tuple_stdout(){
	return stdoutput;
}
