#include <stdio.h>
#include <stdlib.h>

int
main(void) {
	int ret;
	
	int fh = open("test.txt",6);

	if (fh<0) {
		printf("open failed...");
		return -1;
	}

	const char* msg = "HERE...";
	int err2 = write(fh,msg,7);

	if (err2<0) {
		printf("write failed...\n");
		return -1;
	}
	
	char* c = malloc(sizeof(char)*7);
	int err3 = read(ret,c,7);

	if (err3<0) {
		printf("read failed...\n");
		return -1;
	}

	int r = strcmp(msg,c);

	if (!r) {
		printf("It all went terribly, terribly wrong...\n");
		return -1;
	}
	
	printf("FILE TEST PASSED :-)\n\n");
	return 0;
}
