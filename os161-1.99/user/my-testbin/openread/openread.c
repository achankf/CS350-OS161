#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main() {

	const char * path = "read_test.txt";
	int fd = open(path, O_RDWR); 

	char result[9];
    char *dest;
    dest = result;
    read(fd, dest, 9);
	
    printf("%s", dest);
    write(fd, dest, 9);
    
    close(fd);

    fd = open(path, O_RDWR);
    char result2[9];
    char *dest2 = result2;
    read(fd,dest2,5);

    printf(dest2);

    write(fd, dest,9);

    close(fd);


    return 0;
}


