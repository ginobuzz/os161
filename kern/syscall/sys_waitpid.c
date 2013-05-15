/**************************************************************************
 * wait_pid
 *
 * Input:
 *		pid 	- PID of target process.
 *		status* - Return value; status of exited process.
 *		options - Only accepts 0 argument.
 *		retval* - Set to target PID if successful.
 * 
 * Output:
 *		pid_t 	- 0 if successful. Error code if failed.
 *
 * Description:
 *		Waits for the process with the target PID if interested. After the
 *		target process exits - destroys it.
 *		Only interested if the target is the direct child of the current
 *		process. 
 *
 * Last Modified: 5/14/2013 by ginobuzz
 *
 **************************************************************************/
#include <kern/wait.h>
#include <syscall.h>



pid_t
sys_waitpid(pid_t pid, int* status, int options, int* retval) {
	*status = -1;

	/* Check input */
	if (options != 0) 	return EINVAL;
	if (status == NULL) return EFAULT;
	if (pid == 0)		return ESRCH;


	/* Get reference to targeted process */
	struct proc* target = get_proc(pid);
	if (target == NULL)	return ESRCH;


	/* Check if interested in target process */
	if(target->ppid != curthread->process->pid) return ECHILD;


	/* Wait until target process exits */
	if(target->status == STATUS_ALIVE) P(target->sem);
	KASSERT(target->status == STATUS_EXITED);
	
	/* Set results */
	*status = _MKWAIT_EXIT(target->exit_code);
	*retval = pid;

	/* Destroy target process */
	proc_destroy(pid);

	
	return 0;
}
