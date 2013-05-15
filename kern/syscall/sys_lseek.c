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
#include <kern/stat.h>
#include <kern/seek.h>

int sys_lseek(int filehandle, off_t position, int whence, off_t* retval) {
	// check to see if filehandle is valid
	if (filehandle<0 || filehandle>=MAX_FILES) {	
		return EBADF;
	}

	// get the appropriate fdesc, and make sure it is valid
	struct fdesc* fd = curthread->process->ftable[filehandle];
	if (fd == NULL) {
		return EBADF;
	}

	struct stat s;
	VOP_STAT(fd->fobject,&s);
	int deverr = s.st_rdev;

	if (deverr) { // console devices do not support seeking
		return ESPIPE;
	}

	off_t new_offset;

	switch (whence) {
	
		case SEEK_SET:
			new_offset = position;
			break;
		
		case SEEK_CUR:
			new_offset = fd->offset + position;
			break;

		case SEEK_END: {
			new_offset = s.st_size + position;
			break;
		}
		default:
			return EINVAL;
	}

	

	// new offset would be negative
	if (new_offset<0) {
		return EINVAL;
	}

	// everything is valid, so set the offset to the new value and return success
	fd->offset = new_offset;

	*retval = new_offset;

	return 0;
}
