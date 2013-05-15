/**************************************************************************
 * get_pid
 * 
 * Output:
 *		pid_t 	- pid of the current thread's process.
 *
 * Description:
 *		Returns the current process's PID. Cannot fail.
 *
 * Last Modified: 5/14/2013 by ginobuzz
 *
 **************************************************************************/
#include <syscall.h>


pid_t
sys_getpid(void) {
	
	return curthread->process->pid;

}


