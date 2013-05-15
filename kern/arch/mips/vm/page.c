#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <thread.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <mips/vm.h>
#include <mainbus.h>
#include <mips/mivm.h>
#include <mips/coremap.h>
#include <syscall.h>
#include <page.h>
#include <vnode.h>
#include <mips/page.h>
#include <uio.h>
#include <kern/iovec.h>
//#include <spl.h>

struct lock* l;
struct lock* l2;

// copy page contents from one page IN MEMORY to another page IN MEMORY (no swapping support yet)
struct page* 
page_copy(struct page* src) {

	//lock_acquire(l2);
	
	KASSERT(src != NULL);

	struct page* dest = page_create();	// page_create disables interrupts
	KASSERT(dest != NULL);
	
	// Check if source-page is allocated.

	spinlock_acquire(&src->pg_lock);

		if (src->status != IN_MEM) {
			//lock_release(l2);
			spinlock_release(&src->pg_lock);		
			return dest;
		}

	spinlock_release(&src->pg_lock);
	
	spinlock_acquire(&dest->pg_lock);

		dest->ram_addr = coremap_ualloc(dest);

		KASSERT((dest->ram_addr & PAGE_FRAME) == dest->ram_addr);

		dest->status = IN_MEM;

		KASSERT(dest->ram_addr != 0);
		KASSERT(src->ram_addr != 0);

	
	spinlock_acquire(&src->pg_lock);

	coremap_copy(dest->ram_addr,src->ram_addr);

	spinlock_release(&src->pg_lock);
	spinlock_release(&dest->pg_lock);

	//lock_release(l2);

	return dest;
}


// destroy the struct page and free the cm_entry
void 
page_destroy(struct page* p) {
	//lock_acquire(l);

		if (p->status == IN_MEM) {
			coremap_ufree(p->ram_addr);
		}

		p->ram_addr = 31;

		spinlock_cleanup(&p->pg_lock);

	//lock_release(l);
}

// create & initialize a page
struct page* 
page_create(void) {
	int spl = splhigh();

		if (l==NULL)
			l = lock_create("page lock");

		if (l2==NULL)
			l2 = lock_create("page copy lock");

		struct page* p = kmalloc(sizeof(struct page));
		if(p == NULL) {
			splx(spl);
			return NULL;
		}

		p->status = NOT_ALLOCD;
		p->hd_offset = 0;
		p->ram_addr = 31;
		p->is_dirty = 0;
		spinlock_init(&p->pg_lock);

	splx(spl);
	return p;
}

int
page_alloc(struct page* p) {
	spinlock_acquire(&p->pg_lock);

		p->ram_addr = coremap_ualloc(p);

		if (p->ram_addr == 0) {
			spinlock_release(&p->pg_lock);
			return ENOMEM;
		}

		p->status = IN_MEM;
	
	spinlock_release(&p->pg_lock);

	return 0;
}

void
page_free(struct page* p) {
	spinlock_acquire(&p->pg_lock);

		if (p->status == IN_MEM) {
			coremap_ufree(p->ram_addr);
			p->status = NOT_ALLOCD;
			p->is_dirty = 0;
		}

	spinlock_release(&p->pg_lock);
}


int write_out_page(struct page* p) {
	if (p->hd_offset==0) {
		p->hd_offset = next_offset;
		next_offset += PAGE_SIZE;
	}

	struct uio u;
	struct iovec iov;
	uio_kinit(&iov,&u,(void*)PADDR_TO_KVADDR(p->ram_addr),PAGE_SIZE,p->hd_offset,UIO_WRITE);

	int res = VOP_WRITE(swapspace,&u);
	return res;
}

int read_in_page(struct page* p) {
	struct uio u;
	struct iovec iov;
	uio_kinit(&iov,&u,(void*)PADDR_TO_KVADDR(p->ram_addr),PAGE_SIZE,p->hd_offset,UIO_READ);

	int res = VOP_READ(swapspace,&u);
	return res;
}

struct page* find_swapout(void) {
	struct page* p = coremap_find_swapout();
	// TLB shootdown
	return p;
}
