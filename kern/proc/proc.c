#include <proc.h>


/* Global Process List. */
struct proc* ptable[MAX_PROCS];


/* Process Table Lock */
struct lock* proc_lock;




/*  
 *  Init function to be called only once, at the start of the OS.
 *  Creates a global proc list and an init proc as its first entry.
 *  Binds stdin, stdout, and stderr to global fdescs in file.h.
 *  This function will cause a panic if it errors, this was a
 *  design choice.
*/  int proc_init(void) {
	
	// Create process table lock.
	proc_lock = lock_create("Process Table Lock");
	if(proc_lock == NULL) {
		panic("[ERROR] Ptable lock failed to created.\n");
	}

	// Creat init processs.
	pid_t pid = proc_create();
	if(pid < 0) {
		panic("[ERROR] Init process failed to create.\n");
	}

	curthread->parent = get_proc(pid);

	// Define path to open std fdescs.
	char str[] = "con:";
	char * path = kmalloc(sizeof(str));	
	int err;

	// Create STDIN
	//***************************************************************************
	ptable[pid]->ftable[0] = kmalloc(sizeof(struct fdesc));
	if(ptable[pid]->ftable[0] == NULL) {
		panic("[ERROR] stdin failed to allocate memory.\n");
	}
	ptable[pid]->ftable[0]->offset = 0;
	ptable[pid]->ftable[0]->ref_count = 1;
	ptable[pid]->ftable[0]->flags = O_RDONLY;
	ptable[pid]->ftable[0]->lock = lock_create("File Descriptor Lock");
	if(ptable[pid]->ftable[0]->lock == NULL) {// Check.	
		panic("[ERROR] stdin lock failed to create.\n");
	}
	strcpy(path, str);	
	err = vfs_open(path, O_RDWR, 0, &ptable[pid]->ftable[0]->fobject);
	if(err != 0) {
		panic("ERROR: stdin failed to create vnode.\n");	
	}
	//***************************************************************************


	// Create STDOUT
	//***************************************************************************
	ptable[pid]->ftable[1] = kmalloc(sizeof(struct fdesc));
	if(ptable[pid]->ftable[1] == NULL) {
		panic("[ERROR] stdout failed to allocate memory.\n");
	}
	ptable[pid]->ftable[1]->offset = 0;
	ptable[pid]->ftable[1]->ref_count = 1;
	ptable[pid]->ftable[1]->flags = O_WRONLY;
	ptable[pid]->ftable[1]->lock = lock_create("File Descriptor Lock");
	if(ptable[pid]->ftable[1]->lock == NULL) {// Check.	
		panic("[ERROR] stdout lock failed to create.\n");
	}
	strcpy(path, str);	
	err = vfs_open(path, O_RDWR, 0, &ptable[pid]->ftable[1]->fobject);
	if(err != 0) {
		panic("ERROR: stdout failed to create vnode.\n");	
	}
	//***************************************************************************


	// Create STDERR
	//***************************************************************************
	ptable[pid]->ftable[2] = kmalloc(sizeof(struct fdesc));
	if(ptable[pid]->ftable[2] == NULL) {
		panic("[ERROR] stderr failed to allocate memory.\n");
	}
	ptable[pid]->ftable[2]->offset = 0;
	ptable[pid]->ftable[2]->ref_count = 1;
	ptable[pid]->ftable[2]->flags = O_RDWR;
	ptable[pid]->ftable[2]->lock = lock_create("File Descriptor Lock");
	if(ptable[pid]->ftable[2]->lock == NULL) {// Check.	
		panic("[ERROR] stderr lock failed to create.\n");
	}
	strcpy(path, str);	
	err = vfs_open(path, O_RDWR, 0, &ptable[pid]->ftable[2]->fobject);
	if(err != 0) {
		panic("ERROR: stderr failed to create vnode.\n");	
	}
	//***************************************************************************

	kfree(path);
	return 0;
}



pid_t
proc_init2(void) {
	
	pid_t pid = proc_create();
	if(pid < 0) {
		panic("[ERROR] Second process failed to create. Error #%d.\n", pid);
	}
	
	// Clone from the parent.
	proc_clone(0,pid);

	return pid;
}


pid_t
proc_create(void) {
/* Allocates space for a new process in the ptable and initializes it. Returns the PID
 * of the new process if it was successful and returns < 0 otherwise.
*/
	pid_t pid = -1;
	int i;
	//KASSERT(curthread->parent->pid==1);

	// Lock the process table!
	lock_acquire(proc_lock);
	//KASSERT(curthread->parent->pid==1);
	// Check lock.
	if(!lock_do_i_hold(proc_lock)){
		panic("Proc_Create could not get lock.");
		return -1;
	}
	//KASSERT(curthread->parent->pid==1);

	// Find available PID. 
	for (i = 0; i < MAX_PROCS; i++) {
		if(ptable[i] == NULL) {
			pid = i;
			break;		
		}
	//KASSERT(curthread->parent->pid==1);
	}// Check available PID was found.
	if (pid == -1) { // Check.
		lock_release(proc_lock);
		return -1; // ERROR: No available pid.
	}
	//KASSERT(curthread->parent->pid==1);

	// else....

	// Allocate space for new process.
	ptable[pid] = kmalloc(sizeof(struct proc));
	//KASSERT(curthread->parent->pid==1);
	if(ptable[pid] == NULL) { // Check.
		lock_release(proc_lock);
		return ENOMEM; //ERROR: No memory.
	}
	//KASSERT(curthread->parent->pid==1);

	// Initialize pid.
	ptable[pid]->pid  = pid;
	//KASSERT(curthread->parent->pid==1);

	// Initialize parent pid to placeholder value.
	ptable[pid]->ppid = -1; 
	//KASSERT(curthread->parent->pid==1);

	// Initialize exitcode to -1, not yet exited.
	ptable[pid]->exit_code = -1; 
	//KASSERT(curthread->parent->pid==1);

	// Initialize status to ALIVE.
	ptable[pid]->status = STATUS_ALIVE;
	//KASSERT(curthread->parent->pid==1);

	// Initialize semaphore.
	ptable[pid]->sem = sem_create("Process Semaphore",0);
	//KASSERT(curthread->parent->pid==1);
	if(ptable[pid]->sem == NULL){
		kfree(ptable[pid]);
		lock_release(proc_lock);
		return -1;
	}
	//KASSERT(curthread->parent->pid==1);

	// Initialize the filetable.
	for(i = 0; i < MAX_FILES; i++) {
	//KASSERT(curthread->parent->pid==1);
		ptable[pid]->ftable[i] = NULL;
	}
	//KASSERT(curthread->parent->pid==1);

	//KASSERT(curthread->parent->pid==1);

	//kprintf("\n<< Process %d Created. >> \n\n", ptable[pid]->pid);

	// Release the lock
	lock_release(proc_lock);
	//KASSERT(curthread->parent->pid==1);
	return pid;
}


int
proc_clone(pid_t parent, pid_t child) {
/*  Clones the filetable from the parent to the child.
 *  Sets the pid and ppid. Copies over the filetable.
*/
	int i;

	// Lock the process table!
	lock_acquire(proc_lock);

	ptable[child]->ppid = parent;

	// Clone filetable.	
	for(i = 0; i < MAX_FILES; i++) {
		fdesc_clone(ptable[parent]->ftable[i], &ptable[child]->ftable[i]);
	}

	// Unlock the process table.
	lock_release(proc_lock);

	return 0;
}


void
proc_destroy(pid_t pid) {
/* Destroys a target process.
*/
 	pid_t parent_pid;
 	int i;

 	// Lock ptable.
 	lock_acquire(proc_lock);

 	ptable[pid]->status = STATUS_EXITED;// Change status.
 	ptable[pid]->exit_code = 1;// Fatal signal.
 	V(ptable[pid]->sem);// Release any waiting processes.

 	// Set child parent as current parent.
 	parent_pid = ptable[pid]->ppid;// get parent pid.
 	for(i = 1; i < MAX_PROCS; i++) {
 		if(ptable[i] != NULL){
 			if(ptable[i]->ppid == pid) {
 				ptable[i]->ppid = parent_pid;
 			}
 		}
 	}

 	// Close any open fdecs with one reference.
 	for(i = 0; i < MAX_FILES; i++){
		if (ptable[pid]->ftable[i])
 			fdesc_destroy(ptable[pid]->ftable[i]);
 	}

 	// Free remaining memory.
 	sem_destroy(ptable[pid]->sem);
 	kfree(ptable[pid]);
 	ptable[pid] = NULL;

 	//kprintf("\n<< Process %d Destroyed. >> \n\n", pid);

	// Release lock.
 	lock_release(proc_lock);
}


struct proc*
get_proc(pid_t pid){

	struct proc* proc  = NULL;

	if(pid > MAX_PROCS)  return NULL;// Proc DNE.
	else if(pid < 0) 	 return NULL;// Proc DNE.

	lock_acquire(proc_lock);
	
		proc = ptable[pid];
	
	lock_release(proc_lock);

	return proc;
}
