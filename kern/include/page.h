#ifndef _PAGE_H_
#define _PAGE_H_

#include <types.h>
#include <spinlock.h>

// page statuses
#define IN_MEM 		0
#define IN_SWAP 	1
#define NOW_SWAPPING 	2
#define NOT_ALLOCD	3	// page was not allocated in memory, but process has permission to allocate it

struct page {
	unsigned 	status:2;
	off_t 		hd_offset;	// offset into the hard drive, used for swap
	paddr_t 	ram_addr;	// physical address of this page in RAM
	unsigned 	is_dirty:1;	// if the page needs to be written to disk before being swapped out
	struct spinlock	pg_lock;
};

struct page* page_copy(struct page* src); // return NULL for failure
void page_destroy(struct page* p);
struct page* page_create(void);	// return NULL for failure
int page_alloc(struct page* p);
void page_free(struct page* p);

#endif
