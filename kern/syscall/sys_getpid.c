#include <syscall.h>


pid_t
sys_getpid(void) {
/* Returns the PID of the current process.
*/ 
	return curthread->parent->pid;
}


