#ifndef _PROC_H_
#define _PROC_H_

#include <types.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <file.h>
#include <current.h>
#include <thread.h>




#define MAX_PROCS 32
#define MAX_FILES 32

#define STATUS_ALIVE  0
#define STATUS_EXITED 1


/* Process Structure */ 
struct proc {
	pid_t pid; //PID
	pid_t ppid; //Parent PID; 
	struct fdesc* ftable[MAX_FILES]; //Filetable.
	int exit_code; //Exit code returned from the process.
	int status; // Status code of the process.
	struct semaphore* sem;//Semaphore for waitpid.
};


/* Global Process List. */
extern struct proc * ptable[MAX_PROCS];


/* Process Table Lock */
extern struct lock * proc_lock;


//==============================================================
// Function Prototypes
//==============================================================
int 	proc_init(void);
pid_t 	proc_init2(void);

pid_t 	proc_create(void);
int		proc_clone(pid_t parent, pid_t child);
void	proc_destroy(pid_t pid);

struct proc* get_proc(pid_t pid);

#endif
