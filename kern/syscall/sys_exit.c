/**************************************************************************
 * exit
 *
 * Input:
 *		exitcode - exit code of the exiting process.
 *
 * Description:
 *		Causes the current thread's process to exit.
 *
 * Last Modified: 5/15/2013 by ginobuzz
 *
 **************************************************************************/
#include <syscall.h>
#include <kern/wait.h>


void
sys_exit(int exitcode) {

	struct proc* proc = curthread->process;
	KASSERT(proc != NULL);

	proc->exit_code  = exitcode;
	proc->status 	= STATUS_EXITED;

	V(proc->sem);

	thread_exit();
}
