#ifndef _FILE_H_
#define _FILE_H_


#include <types.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <vnode.h>
#include <vfs.h>
#include <synch.h>
#include <array.h>


/* File Descriptor Structure */
struct fdesc {
	int offset;
	int ref_count;
	int flags;
	struct vnode* fobject;
	struct lock* lock;
};

struct fdesc* fdesc_create(int flag, struct vnode* v);
int fdesc_destroy(struct fdesc* fd);
int fdesc_clone(struct fdesc* src, struct fdesc** dest);


#endif



