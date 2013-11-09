#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 4

int main(){
	const char *path = "boo";
 	open(path, O_RDWR | O_APPEND);

	if (errno != EINVAL){
		puts("FAILURE");
	} else {
		puts("OKAY");
	}
	return 0;
}
