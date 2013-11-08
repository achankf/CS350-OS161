#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
	puts("\nExecv test\n");
	const char *args[] = { "zero", "one", "two", NULL };
	int result;
	result = execv("testbin/argtest", (char **) args);
	if(result)
		puts("\nexecv jikkou shippai shichatta\n");
}
