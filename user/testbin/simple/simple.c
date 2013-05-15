#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int
main(void) {

	int ret = fork();

	if( ret == 0 ) {
		printf( "I am the child. Going to return 10. My pid is %d\n", getpid() );
		return 10;
	}
	else {
		int err, status;
		printf( "I am the parent. My PID is %d\nWaiting on %d\n", getpid(), ret );
		err = waitpid( ret, &status, 0 );
		if( err != ret ) {
			printf( "ERROR. Bad WAITPID return. status:%d err:%d, ret:%d\n",status,err,ret);
			return -1;
		}

		printf( "Child died with %d\n", status );
		return 0;
		
	}
	return 0;
}
