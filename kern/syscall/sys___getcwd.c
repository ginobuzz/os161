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


int sys___getcwd(char* buf, size_t size, int32_t* retval) {
	char* kbuf;
	int cpyerr = copyin((const_userptr_t)buf,kbuf,sizeof(buf));
	if (cpyerr)
		return cpyerr;

	struct uio u;
	struct iovec iov;

	uio_kinit(&iov,&u,buf,size,0,UIO_READ);
	
	int err = vfs_getcwd(&u);

	if (err) {
		return err;
	}

	*retval = size;

	return 0;
}
