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

int sys_dup2(int old_filehandle, int new_filehandle, int32_t* retval) {
	// if both filehandles are the same, do nothing
	if (old_filehandle == new_filehandle) {
		*retval = new_filehandle;
		return 0;
	}

	// check to make sure both file handles are valid
	if (old_filehandle<0 || old_filehandle>MAX_FILES) {
		return EBADF;
	}
	if (new_filehandle<0 || new_filehandle>MAX_FILES) {
		return EBADF;
	}

	// get the respective fdesc's from the file table
	struct fdesc* fd_old = curthread->parent->ftable[old_filehandle];
	struct fdesc* fd_new = curthread->parent->ftable[new_filehandle];

	// make sure old fdesc is valid
	if (fd_old==NULL) {
		return EBADF;
	}

	// check to see if fd_new is an open file, and close it if it is
	if (fd_new != NULL) {
		int err = sys_close(new_filehandle);
		if (err) {
			return err;
		}
	}

	// set the old fdesc to the new fdesc
	curthread->parent->ftable[new_filehandle] = fd_old;	
	fd_old->ref_count++;

	// return the new filehandle
	*retval = new_filehandle;
	return 0;
}
