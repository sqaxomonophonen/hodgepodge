// cc -Wall ref.c -o ref -pthread && strace -o ref.strace ./ref
// this program is mostly here for me to see what threads look like in strace
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

static void* thread_start(void *arg)
{
	for (int i = 0; i < 5; i++) {
		usleep(100000);
		printf("%d\n", i);
	}
	return NULL;
}

int main(int argc, char** argv)
{
	pthread_t tid;
	printf("begin\n");
	assert(0 == pthread_create(&tid, NULL, &thread_start, NULL));
	printf("begun\n");
	assert(0 == pthread_join(tid, NULL));
	printf("end\n");
	return 0;
}
