#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <uio.h>
#include <kern/iovec.h>
#include <vfs.h>
#include <file.h>
#include <kern/errno.h>
#include <proc.h>
#include <addrspace.h>
#include <copyinout.h>
#include <limits.h>

int sys_open(const char *filename, int flags, int32_t* retval) {
	if (filename==NULL) {
		return EFAULT;
	}

	struct fdesc* fd;

	// make sure we have an open file table index before we do anything else
	int i = 0;
	while(curthread->process->ftable[i] != NULL && i<MAX_FILES) {
		i++;
	}

	if (i>=MAX_FILES) { // file table is full
		return EMFILE;
	} 


	int rw_permissions;

	//kprintf("\nflags&O_RDONLY = %d\n",b);
	if (flags == O_RDONLY || (flags&O_RDONLY)) {			// read-only
		rw_permissions = O_RDONLY;
	} else if (flags&O_WRONLY) {		// write-only
		rw_permissions = O_WRONLY;
	} else if (flags&O_RDWR) {		// read-write
		rw_permissions = O_RDWR;
	} else {				// invalid or unsupported flags
		return EINVAL;
	}

	char kbuf[PATH_MAX];

	int cpyerr = copyinstr((const_userptr_t)filename,kbuf,PATH_MAX,NULL);
	if (cpyerr) {
		return cpyerr;
	}

	struct vnode* vn = kmalloc(sizeof(struct vnode));
	if (vn == NULL) {
		return EIO;
	}

	int error = vfs_open((char*)filename,flags,0,&vn);
	if(error) {
		kfree(vn);
		return error;
	}
	
	// everything has succeeded, create the fdesc and return the file handle
	fd = fdesc_create(rw_permissions,vn);

	curthread->process->ftable[i] = fd;	
	
	*retval = i;
	return 0;
}
