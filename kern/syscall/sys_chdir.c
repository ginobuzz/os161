#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <kern/errno.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <uio.h>
#include <kern/iovec.h>
#include <vfs.h>
#include <proc.h>
#include <addrspace.h>
#include <copyinout.h>
#include <file.h>


int sys_chdir(char* path) {
	if (path == NULL) {
		return EFAULT;
	} else if (path=="") {
		return EINVAL;

	}

	char* kpath;

	int cpyerr = copyin((const_userptr_t)path,kpath,sizeof(path));
	if (cpyerr) {
		kprintf("error in copyin...\n");
		return cpyerr;
	}

	// actually change the directory to path
	int err = vfs_chdir(kpath);

	// check if vfs_chdir returned an error
	if (err) {
		return err; // return error to userspace
	}

	return 0;	// success
}
