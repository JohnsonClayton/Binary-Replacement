#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
	int ret = fork();
	if(ret == 0) {
		//Child
		printf("Child: %d\n", getpid());
		execl("hello_world_mal", "", NULL);
	} else {
		//Parent
		printf("Parent: %d\n", getpid());
		//int pid = wait(0);
		//printf("I am %d and %d has been terminated\n",getpid(),  pid);
		int status;
		pid_t termianted = waitpid(-1, &status, 0);
		if(WIFEXITED(status)) {
			printf("%d: child executed\n", getpid());
		} else {
			printf("%d: child terminated\n", getpid());
		}


	}
	puts("Returning!");
	return 0;
}
