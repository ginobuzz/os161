#include <syscall.h>


#define MAX_ARGS 10



int
sys_execv(userptr_t progname, userptr_t argv){

	const int NAME_MAX = __NAME_MAX;
	const int ARG_MAX  = __ARG_MAX;

	char* kprogname;
	char* kargs[MAX_ARGS];
	int ksize[MAX_ARGS];
	int argc = 0;
	int avail_mem = ARG_MAX;
	size_t actual;
	int i, k, err;


	// Check for valid input.
	if(progname == NULL) return EFAULT;
	if(argv == NULL) return EFAULT;


	// Allocate space for kprogname.
	kprogname = kmalloc(NAME_MAX);
	if(kprogname == NULL) return ENOMEM;	
	
	// Copy in progname from user-space.
	err = copyinstr(progname, kprogname, NAME_MAX, &actual);
	if(err){// Check.
		kfree(kprogname);
		return err;
	}
	
	KASSERT(kprogname != NULL);
	if(strlen(kprogname) == 0){
		kfree(kprogname);
		return EINVAL;
	}



	// Copy in args.
	for(argc = 0; ; argc++){

		userptr_t argvptr = argv + (argc * sizeof(char*));
		
		char* tmp = kmalloc(sizeof(char*));
		if(tmp==NULL){
			kfree(kprogname);
			return ENOMEM;
		} 

		KASSERT(argvptr != NULL);

		// Copy in pointer.
		err = copyin(argvptr, &tmp, sizeof(char*));
		if(err){
			for(i = 0; i < argc; i++) kfree(kargs[i]);
			kfree(kprogname);
			return err;
		}

		if(tmp == NULL){
			break;
		}

		// Allocate space for new arg.
		kargs[argc] = kmalloc(avail_mem);
		if(kargs[argc] == NULL){
			for(i = 0; i < argc; i++) kfree(kargs[i]);
			kfree(kprogname);
			return ENOMEM;
		}


		


		// Copy in arg from tmp pointer.
		err = copyinstr((userptr_t)tmp, kargs[argc], avail_mem, &actual);
		if(err){
			for(i = 0; i < argc; i++) kfree(kargs[i]);
			kfree(kprogname);
			return err;
		}
		KASSERT(kargs[argc] != NULL);

		ksize[argc] = actual;// Set size.
		while(ksize[argc]%4 != 0) ++ksize[argc];// Pad size.
		avail_mem -= ksize[argc];// Adjust available memory.
		if(kargs[argc] == NULL){
			break;// Last argument reached.
		}

	}


	struct vnode* v;
	vaddr_t entrypoint, stackptr;
	struct addrspace* backup_addrspace = curthread->t_addrspace;
	/* Open the file. */
	err = vfs_open(kprogname, O_RDONLY, 0, &v);
	if (err) {
		for(i = 0; i < argc; i++) kfree(kargs[i]);
		kfree(kprogname);
		return err;
	}
	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace==NULL) {
		curthread->t_addrspace = backup_addrspace;
		vfs_close(v);
		for(i = 0; i < argc; i++) kfree(kargs[i]);
		kfree(kprogname);
		return ENOMEM;
	}
	/* Activate it. */
	as_activate(curthread->t_addrspace);
	/* Load the executable. */
	err = load_elf(v, &entrypoint);
	if (err) {
		curthread->t_addrspace = backup_addrspace;
		as_activate(curthread->t_addrspace);
		for(i = 0; i < argc; i++) kfree(kargs[i]);
		kfree(kprogname);
		vfs_close(v);
		return err;
	}
	/* Done with the file now. */
	vfs_close(v);
	/* Define the user stack in the address space */
	err = as_define_stack(curthread->t_addrspace, &stackptr);
	if (err) {
		curthread->t_addrspace = backup_addrspace;
		as_activate(curthread->t_addrspace);
		for(i = 0; i < argc; i++) kfree(kargs[i]);
		kfree(kprogname);
		return err;
	}


	int arg_offset = (argc + 1) * sizeof(char*);

	int total_size = arg_offset;
	for(i = 0; i < argc; i++) total_size += ksize[i];

	stackptr -= total_size;
	//KASSERT((stackptr & PAGE_FRAME) == stackptr);

	// Copy out buff.
	for(i = 0; i < argc; i++){
		
		userptr_t src  = (userptr_t)(stackptr + arg_offset);
		userptr_t dest = (userptr_t)(stackptr + (i * sizeof(char*)));

		// Copy out pointer.
		err = copyout(&src, dest, sizeof(char*));
		if(err){
			curthread->t_addrspace = backup_addrspace;
			as_activate(curthread->t_addrspace);
			for(k=0; k<argc; k++) kfree(kargs[k]);
			kfree(progname);
			return err;
		}

		// Copy out the args.
		err = copyoutstr(kargs[i], src, strlen(kargs[i])+1, &actual);
		if(err){
			curthread->t_addrspace = backup_addrspace;
			as_activate(curthread->t_addrspace);
			for(k=0; k<argc; k++) kfree(kargs[k]);
			kfree(progname);
			return err;
		}

		arg_offset += ksize[i];
	}

	// Free used memory;
	for(i = 0; i < argc; i++) kfree(kargs[i]);
	kfree(kprogname);

	as_destroy(backup_addrspace);

	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t)stackptr, stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");

	return EFAULT;

}
