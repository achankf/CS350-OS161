#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 4

static int test_open_read(const char *path){
	char result[BUF_SIZE];
	char *dest = result;
 	int fd = open(path, O_RDWR);
	int err;


	if (fd < 0){
		return 1;
	}

 	//while ((err = read(fd, dest, BUF_SIZE))){
	for(;;){
		err = read(fd,dest,BUF_SIZE);
		if (err == -1) {
			return 1;
		} else if (err == 0){
			break;
		}
 		printf("%s", dest);
	}

	puts("");

 	close(fd);
	return 0;
}

int main(int argc, char **argv) {

	if (argc != 2){
		puts("Requires a file as argument");
		return 1;
	}

	const char *path = argv[1];

	for (int i = 0; i < 1; i++){
		int result = test_open_read(path);
		if (result){
			printf("ERROR:%s\n", strerror(errno));
			return result;
		}
	}

	return 0;
}


