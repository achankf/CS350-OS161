#include <unistd.h>
#include <stdio.h>

int
main()
{
	int rc = fork();

	if (rc == 0){
		// wait for some time
		for (volatile int i = 0; i< 10000000; i++);
		printf("I am the child %d\n", getpid());
	} else if (rc > 0){
		int status;
		printf("I am the parent; waiting for child %d\n", rc);
		int ret = waitpid(rc, &status, 0);
		if (ret){
			puts("Error occurred in waitpid");
		} else {
			printf("Child %d finish %d\n", rc, status);
		}
	} else {
		printf("Error %d\n", rc);
	}
	return 0;
}
