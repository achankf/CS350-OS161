#ifndef _FD_TUPLE_H_
#define _FD_TUPLE_H_
#include <types.h>

struct vnode;

#define FDTABLE_SIZE 10
struct fd_tuple {
	struct vnode *vn;
	off_t offset;
	int counter;
	struct lock *lock; // sync between reads and writes
};

// creates global reference for stdin, stdout, stderr
void fd_tuple_bootstrap(void);

// create a fd_tuple
struct fd_tuple *fd_tuple_create(const char *name, int flags, mode_t mode);

// decrement the counter or delete the destroy up the tuple
void fd_tuple_give_up(struct fd_tuple *);

// return the stdin
struct fd_tuple *fd_tuple_stdin(void);

// return the stdout
struct fd_tuple *fd_tuple_stdout(void);

#endif
