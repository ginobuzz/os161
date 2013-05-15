#include <kern/wait.h>
#include <syscall.h>


pid_t
sys_waitpid(pid_t pid, int* status, int options, int* retval) {


	if (options != 0) return EINVAL;
	if (status == NULL) return EFAULT;


	struct proc* proc = get_proc(pid);

	// Does process exist?
	if(proc == NULL) {
		*status = -1;
		return ESRCH;//Process DNE.
	}


	// Check for pid 0.
	if(pid == 0){
		*status = -1;
		return ESRCH;
	}


	// Are we are interested?
	if(proc->ppid != curthread->parent->pid) {
		*status = -1;
		return ECHILD;//Not interested.
	}


	// Is the process still alive?
	if(proc->status == STATUS_ALIVE) {
			P(proc->sem);// wait until it exits.
	}
	KASSERT(proc->status == STATUS_EXITED);
	
	int sts = _MKWAIT_EXIT(proc->exit_code);

	int err = copyout(&sts, (userptr_t)status, sizeof(int));
	if(err){
		return err;
	}


	proc_destroy(pid);


	*retval = pid;

	return 0;
}
