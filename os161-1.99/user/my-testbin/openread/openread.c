#include <stdio.h>
#include <unistd.h>

int main() {

	/*
	char * path = "read_test.txt";
	int fd = sys_open(path, O_RDONLY,0664);

	char * result;
	
	// read in 5 chars from a file
	if (sys_read(fd, result, 5) == 0) {
		printf(%s, result);
	}
	return 0;
	*/
	int pid = getpid();
	printf("This is my PID:%d\n", pid);
  	return 0;
}
