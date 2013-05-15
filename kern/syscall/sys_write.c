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


int sys_write(int filehandle, const void *buf, size_t size, int32_t* retval) {
	if (filehandle>=MAX_FILES || filehandle<0) {
		return EBADF;
	}
	// get file descriptor from file table
	struct fdesc* fd = curthread->process->ftable[filehandle];
	

	if(curthread->process == NULL){
		kprintf("[ERROR] in sys_write, curthread->parent is NULL.\n");
	}

	else if(fd == NULL){
		//kprintf("[ERROR] in sys_write, curthread->parent->ftable[filehandle] is NULL.\n");
		return EBADF;
	}

	// lock the file descriptor to prevent interference from other processes/threads
	lock_acquire(fd->lock);

	// Check to see if lock was acquired.
	if(!lock_do_i_hold(fd->lock)){
		//kprintf("[ERROR] in sys_write, fd-> lock unavailable.\n");
		return EIO;
	}

	// check if file has write permissions
	if (fd->flags != O_WRONLY && fd->flags != O_RDWR) {
		lock_release(fd->lock);
		return EBADF;
	}

	struct vnode* vn = fd->fobject;

	// create a uio struct to pass to VOP_WRITE
	struct uio u;

	// iovec is needed for uio struct, and holds size and data
	struct iovec iov;


	uio_uinit(&iov,&u, (char*)buf,size, fd->offset, UIO_WRITE, curthread->t_addrspace);

	// actually does the write
	int result = VOP_WRITE(vn,&u);
	if (result) {
		lock_release(fd->lock);
		return result;
	}

	// the number of bytes actually written
	int written = size-u.uio_resid;

	if (written<0) {
		lock_release(fd->lock);
		return EIO;
	}
		

	// increase the offset by the amount that was actually written
	fd->offset += written;

	// return the number of bytes written
	*retval = written;

	// unlcok the file descriptor
	lock_release(fd->lock);

	// success
	return 0;
}
