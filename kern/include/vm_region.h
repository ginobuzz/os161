#ifndef _VM_REGION_H_
#define _VM_REGION_H_

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <page.h>
#include <array.h>
#include <synch.h>


// Define Pagetable Array.
DECLARRAY_BYTYPE(pagetable, struct page);
#ifndef REGION_INLINE
#define REGION_INLINE INLINE
#endif
DEFARRAY_BYTYPE(pagetable, struct page, REGION_INLINE);




struct region{
	vaddr_t base;// Base virtual-address for page table.
	struct 	pagetable* pagetable;// Linked-list of pages.
	struct 	lock* lock;// Region Lock.
};


/*======================= Member Functions =======================*/

// Create and inititalize region. Null if error.
struct region* region_init(vaddr_t base, int npages);

// Clone a region and return a pointer to it. Null if error.
struct region* region_copy(struct region* src);

// Returns the size of the regions pagetable.
int region_size(struct region* vmr);

// Removes a page from the pagetable. Returns !0 for error.
void region_remove(struct region* region, int index);

// Destroys an entire region.
void region_destroy(struct region* region);

// Locates and returns a page, given a virtual address.
struct page* region_fault(struct region* target, vaddr_t faddr);

// Allocates n pages and returns top addr. Returns NULL if fail.
vaddr_t region_alloc(struct region* region, int npages);

// Deallocates n pages and returns top addr. Returns NULL if fail.
vaddr_t region_dealloc(struct region* region, int npages);

// Returns top of pagetable.
vaddr_t region_gettop(struct region* region);

#endif /* */

