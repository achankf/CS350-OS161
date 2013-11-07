#include <fd_tuple.h>
#include <vfs.h>
#include <vnode.h>
#include <lib.h>
#include <synch.h>
#include <kern/fcntl.h>

struct fd_tuple *stdinput, *stdoutput;

struct fd_tuple *fd_tuple_create(const char *filename, int flags, mode_t mode){

	struct vnode *vn;
	struct fd_tuple *node = kmalloc(sizeof(struct fd_tuple));

	if (node == NULL) return NULL;

	node->lock = lock_create("");

	if (node->lock == NULL){
		kfree(node);
		return NULL;
	}

	char *filenamecopy = kstrdup(filename);
	if (filenamecopy == NULL){
		lock_destroy(node->lock);
		kfree(node);
		return NULL;
	}

	int result = vfs_open(filenamecopy, flags, mode, &vn);
	kfree(filenamecopy);

	if (result){
		lock_destroy(node->lock);
		kfree(node);
		return NULL;
	}

	node->vn = vn;
	node->offset = 0;
	node->counter = 1;

	return node;
}

static void fd_tuple_destroy(struct fd_tuple *tuple){
	VOP_DECREF(tuple->vn);
	lock_destroy(tuple->lock);
	kfree(tuple);
}

void fd_tuple_give_up(struct fd_tuple *tuple){
	lock_acquire(tuple->lock);
		if (tuple->counter > 0){
			tuple->counter++;
		} else {
			fd_tuple_destroy(tuple);
		}
	lock_release(tuple->lock);
}

void fd_tuple_bootstrap(){
	stdinput = fd_tuple_create("con:", O_RDONLY, 0644);
	KASSERT(stdinput != NULL);
	stdoutput = fd_tuple_create("con:", O_WRONLY, 0644);
	KASSERT(stdoutput != NULL);
}

struct fd_tuple *fd_tuple_stdin(){
	return stdinput;
}

struct fd_tuple *fd_tuple_stdout(){
	return stdoutput;
}
