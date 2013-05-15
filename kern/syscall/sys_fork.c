#include <syscall.h>
#include <machine/trapframe.h>
#include <vm.h>
#include <vfs.h>
/*
	Create semaphore
	Clone trapframe.
	Create new process.
		Clone process.
	Set new process parent to current process.
	Initialize fork_struct
	Fork thread
	P semaphore.

	Copy parent’s trap frame, and pass it to child thread
	Copy parent’s address space
	Creat child thread
	Copy parent’s file table into child
	Parent returns with child’s pid immediately
	Child returns with 0
*/


struct fork_struct {
	pid_t c_pid;
	struct trapframe *trapf;
	struct addrspace *addrs;
};



static
void
entrypoint(void* arg1, unsigned long arg2) {
	(void) arg2; // unused.

	struct fork_struct* data = arg1;
	struct trapframe new_tf;

	// Set child process's parent process.
	curthread->process = get_proc(data->c_pid);	

	// Copy address space.
	curthread->t_addrspace = data->addrs;
	as_activate(curthread->t_addrspace);

	// Copy the trapframe.
	memcpy(&new_tf, data->trapf, sizeof(struct trapframe));

	// kfree the data struct.

	kfree(data->trapf);
	kfree(data);

	// Change trapframe values.
	new_tf.tf_a3 = 0;
	new_tf.tf_v0 = 0;
	new_tf.tf_epc += 4;

	mips_usermode(&new_tf);
}





pid_t
sys_fork(struct trapframe* tf, int32_t* retval) {

	pid_t	parent_pid;
	pid_t 	child_pid;
	int 	err;

	// Get current process's pid.
	parent_pid = curthread->process->pid;

	// Create a new process (child).
	child_pid = proc_create();
	if(child_pid < 0) {// Check.
		*retval = -1;
		return ENPROC;
	}

	//kprintf("sys_fork: Child = %d.\n",child_pid);

	// Clone parent process to child.
	err = proc_clone(parent_pid, child_pid);
	if(err != 0){// Check.
		*retval = -1;
		proc_destroy(child_pid);
		return ENOMEM;
	}

	// Create fork_struct to pass to child entrypoint function.
	struct fork_struct *data = kmalloc( sizeof( struct fork_struct ));
	if( data == NULL ) { 
		*retval = -1;
		proc_destroy(child_pid);
		return ENOMEM;
	}

	data->c_pid = child_pid;
	data->trapf = kmalloc( sizeof( struct trapframe ) );
	
	if( data->trapf == NULL ) {
		//cleanup
		return ENOMEM;
	}

	memcpy(data->trapf, tf, sizeof(struct trapframe));
	
	err = as_copy(curthread->t_addrspace, &data->addrs);
	if( err ) {
		*retval = -1;
		proc_destroy(child_pid);
		return ENOMEM;
	}

	//kprintf(curthread->t_name);
	// Fork the parent thread.
	err = thread_fork(curthread->t_name, entrypoint, data, 0, NULL);

	if(err != 0){// Check.
		*retval = -1;
		proc_destroy(child_pid);
		return ENOMEM;
	}

	// Wait for child to finish.
	*retval = (int32_t)child_pid;
	return 0;
}



