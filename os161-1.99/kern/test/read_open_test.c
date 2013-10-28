#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>

static void read_test() {
	char * path = "";
	int fd = sys_open(path, O_RDONLY);

	char * result;
	
	// read in 5 chars from a file
	if (sys_read(fd, result, 5) == 0) {
		kprintf(%s, result);
	}
}