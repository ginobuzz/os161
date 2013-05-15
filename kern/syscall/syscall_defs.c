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

int sys_read(int filehandle, void *buf, size_t size) {
	struct vnode* vn = curthread->t_ftable->table[filehandle]->v;

	//make a struct uio to pass to VOP_READ
	struct uio* u = kmalloc(sizeof(struct uio));

	//need to make a struct iovec
	struct iovec* iov = kmalloc(sizeof(struct iovec));
	iov->iov_ubase = buf;
	iov->iov_len = size;

	u->uio_iov = iov;
	u->uio_iovcnt = 1;
		
	u->uio_offset = (unsigned)curthread->t_ftable->table[filehandle]->offset;
	u->uio_resid = size;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = UIO_READ;
	u->uio_space = curthread->t_addrspace;
	
	VOP_READ(vn,u);
	//(void)filehandle;
	//(void)buf;
	//(void)size;

	return 0;  //success
}

int sys_write(int filehandle, const void *buf, size_t size) {
	kprintf("\n\nHERE...\n\n");
	struct vnode* vn = curthread->t_ftable->table[filehandle]->v;
	struct uio* u = kmalloc(sizeof(struct uio));

	struct iovec* iov = kmalloc(sizeof(struct iovec));
	iov->iov_ubase = (void*)buf;
	iov->iov_len = size;

	u->uio_iov = iov;
	u->uio_iovcnt = 1;
		
	u->uio_offset = (unsigned)curthread->t_ftable->table[filehandle]->offset;
	u->uio_resid = size;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = UIO_WRITE;
	u->uio_space = curthread->t_addrspace;


	//(void)filehandle;
	//(void)buf;
	//(void)size;

	VOP_WRITE(vn,u);

	return 0;
}

int sys_open(const char *filename, int flags) {
	struct vnode* vn = kmalloc(sizeof(struct vnode));

	int error = vfs_open((char*)filename,flags,0,&vn);

	if(error)
		return error;
	
	struct fdesc* fd = kmalloc(sizeof(struct fdesc));
	fdesc_init(fd,vn,flags);
	
	int i;
	for (i=0;i<MAX_FILES;i++) {
		if(curthread->t_ftable->table[i] == NULL) {
			curthread->t_ftable->table[i] = fd;
			return 0;
		}
	}
	
	return EMFILE;
}

pid_t sys_fork(void) { //return 0 to child
	const char* name = "Child of thread"+curthread->tid;
	struct thread* new_thread = kmalloc(sizeof(struct thread));
	thread_fork(name,sys_fork_helper,(void*)curthread->t_addrspace,(unsigned long)curthread->tid,&new_thread);
	return (pid_t)new_thread->tid;
}

void sys_fork_helper(void *data1, unsigned long parent_tid) {
	struct addrspace* adrspace = data1;
	curthread->ptid = (int)parent_tid;
	curthread->tid = get_new_tid();
	as_copy(curthread->t_addrspace,&adrspace);
	curthread->t_ftable = ftable_create();

	//return 0;
}
