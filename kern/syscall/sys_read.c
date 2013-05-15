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


int sys_read(int filehandle, void *buf, size_t size, int32_t* retval) {
	if (filehandle>=MAX_FILES || filehandle<0) {
		return EBADF;
	}

	if (buf==NULL)
		return EFAULT;
/*
	size_t stoplen;

	int cpychk = copycheck((const_userptr_t)buf,size,&stoplen);
	if (cpychk)
		return cpychk;
*/
	// obtain the desired file descriptor from the process file table
	struct fdesc* fd = curthread->process->ftable[filehandle];
	if (fd == NULL) {
		return EBADF;
	}

	// lock the file descriptor
	lock_acquire(fd->lock);

	// Check to see if lock was acquired.
	if(!lock_do_i_hold(fd->lock)){
		//kprintf("[ERROR] in sys_read, fd-> lock unavailable.\n");
		return EIO;
	}


	// check to make sure file has read permissions
	if (fd->flags != O_RDONLY && fd->flags != O_RDWR) {
		//kprintf("[ERROR] in sys_read, wrong permissions.\n");
		lock_release(fd->lock);
		return EBADF;
	}

	struct vnode* vn = fd->fobject;
	
	struct uio u;
	struct iovec iov;

	uio_uinit(&iov,&u,buf,size,fd->offset,UIO_READ,curthread->t_addrspace);
	u.uio_segflg = UIO_USERSPACE;
	
	// actually performs the read, and stores the data in kbuf
	int result = VOP_READ(vn,&u);
	if (result) {
		lock_release(fd->lock);
		return result;
	}

	// the number of bytes actually read
	int read = size-u.uio_resid;

	if (read<0) {
		//kprintf("[ERROR] in sys_read, read < 0.\n");
		lock_release(fd->lock);
		return EIO;
	}
	

	fd->offset += read;

	*retval = read;

	lock_release(fd->lock);

	return 0;  //success
}
