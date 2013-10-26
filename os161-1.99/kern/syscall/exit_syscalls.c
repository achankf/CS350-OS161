#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <thread.h>

int sys__exit(int exit_code)
{
	thread_exit();

	// TODO make exit_code available to "interested" processes
	(void) exit_code; // avoid compiler warning

	panic("sys__exit: thread_exit should not return");
	return -1;
}
