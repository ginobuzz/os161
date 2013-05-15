/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <kern/unistd.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <types.h>
#include <lib.h>
#include <proc.h>
#include <addrspace.h>
#include <thread.h>
#include <current.h>
#include <copyinout.h>
#include <vm_region.h>


struct trapframe;


/* Syscall dispatcher */
void syscall(struct trapframe *tf);


/* Helper for sys_fork */
void enter_forked_process(struct trapframe *tf);

/* Enter user mode. Does not return. */
void enter_new_process(int argc, userptr_t argv, vaddr_t stackptr,
		       vaddr_t entrypoint);


/* Kernel system-call prototypes */
int 	sys_reboot(int code);
int 	sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);
pid_t 	sys_fork(struct trapframe* tf, int* retval);
pid_t 	sys_execv(userptr_t progname, userptr_t args);
pid_t 	sys_getpid(void);
pid_t	sys_waitpid(pid_t pid, int *status, int options, int* retval);
void	sys_exit(int exitcode);
int 	sys_sbrk(intptr_t amount, int32_t* retval);
int 	sys_open(const char *filename, int flags, int32_t* retval);
int 	sys_read(int filehandle, void *buf, size_t size, int32_t* retval);
int 	sys_write(int filehandle, const void *buf, size_t size, int32_t* retval);
int 	sys_dup2(int old_filehandle, int new_filehandle, int32_t* retval);
int 	sys_close(int filehandle);
int 	sys_lseek(int filehandle, off_t position, int whence, off_t* retval);
int 	sys_chdir(char* path);
int 	sys___getcwd(char* buf, size_t size, int32_t* retval);


#endif /* _SYSCALL_H_ */
