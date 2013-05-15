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

int sys_close(int filehandle) {
	if (filehandle>=MAX_FILES || filehandle<0) {
		return EBADF;
	}

	struct fdesc* fd = curthread->process->ftable[filehandle];

	if (fd==NULL)
		return EBADF;

	fdesc_destroy(fd);
	curthread->process->ftable[filehandle] = NULL;

	//if (fd->ref_count < 0) 
	//	panic("sys_close yielded a ref_count of less than 0\n");
	
	//curthread->parent->ftable[filehandle] = NULL;

	return 0;
}
