#include <file.h>

#define ENOMEM -1

struct fdesc*
fdesc_create(int flag, struct vnode* v) {
/* Creates a new file descriptor and initializes
 * its values. Returns NULL if there was an error.
*/

 	struct fdesc* fd = kmalloc(sizeof(struct fdesc));
	if(fd == NULL) {
		return NULL; // Error: Out of Memory.
	}

	// Initialize values.	
	fd->offset = 0;
	fd->ref_count = 1;
	fd->flags = flag;
	fd->fobject = v;

	// Allocate memory for lock.
	fd->lock = lock_create((const char*)"File Descriptor Lock");
	if(fd->lock == NULL) {// Check.	
		kfree(fd);//Free fdesc.
		return NULL; // Error: Out of Memory.
	}
	
	return fd;
}



int 
fdesc_destroy(struct fdesc* fd) {
	
	// Does it exist?
	if(fd == NULL)
		return -1; // D.N.E.

	// Acquire lock.
	lock_acquire(fd->lock);

	--fd->ref_count;
	//VOP_DECREF(fd->fobject);

	// Is this the final fdesc pointer?
	if(fd->ref_count == 0){
		vfs_close(fd->fobject);
		lock_release(fd->lock);
		lock_destroy(fd->lock);
		kfree(fd);
		return 0;
	}

	// Release lock.
	lock_release(fd->lock);

	return 0;
}




int 
fdesc_clone(struct fdesc* src, struct fdesc** dest){
	
	// Does it exist?
	if(src == NULL)
		return -1; // D.N.E.

	// Get lock.
	lock_acquire(src->lock);

	// Copy over the fdesc reference.
	*dest = src;

	// Increment ref_count;
	src->ref_count++;
	VOP_INCREF(src->fobject);

	// Free lock.
	lock_release(src->lock);

	return 0;
}
