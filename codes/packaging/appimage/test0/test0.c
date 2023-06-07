#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv, char **envp)
{
	printf("hello from C!\n");
	for (char** e = envp; *e; e++) {
		printf("  %s\n", *e);
	}
	return EXIT_SUCCESS;
}
