#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
	for (int i = 0; i < 2; i++){
		int rc = fork();
		printf("\t%d\n", rc);
		int ret;
		if (rc > 0){
			waitpid(rc, &ret, 0);
			printf("returned %d\n", ret);
		} else {
			puts("CHILD");
			exit(getpid());
		}
	}
	puts("DONE");
	return 0;
}
