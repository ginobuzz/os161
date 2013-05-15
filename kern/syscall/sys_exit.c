#include <syscall.h>
#include <kern/wait.h>


void
sys_exit(int exitcode) {
/* Causes the current process to exit. 
*/

	KASSERT(curthread->t_addrspace != NULL);

	struct proc* proc = curthread->parent;

	//Set exitcode.
	proc->exit_code = exitcode;

	// Change status.
	proc->status = STATUS_EXITED;

	// Increment processes semaphore, releasing waiting processes.
	V(proc->sem);

	thread_exit();
}
