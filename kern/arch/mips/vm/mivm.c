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
#include <addrspace.h>

/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock tlb_lock = SPINLOCK_INITIALIZER;

void vm_bootstrap() {
	
	swap_initialized = 0;

	paddr_t ram_hi;
	paddr_t ram_low;

	ram_getsize(&ram_low, &ram_hi);	// get the current size of ram

	int ram_pages = (ram_hi-ram_low)/PAGE_SIZE;	// find out how many pages of ram we have

	int cm_bytes = ram_pages*sizeof(struct cm_entry);	// how many bytes our coremap will need to be

	while (cm_bytes % PAGE_SIZE != 0)	// page align the value
		cm_bytes++;

	int cm_pages = cm_bytes/PAGE_SIZE;	// number of pages we need to steal for the coremap

	ram_stealmem(cm_pages);

	paddr_t ram_start = ram_low + (cm_pages*PAGE_SIZE);	// ram will then start at address right after coremap

	ram_pages -= cm_pages;		// don't want the coremap to map to itself

	coremap_init(ram_pages,ram_start,ram_low);	// initialize the coremap

	vm_has_bootstrapped = 1;	// vm has finished initializing
}

// get kernel pages before VM bootstrapps
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);
	
	spinlock_release(&stealmem_lock);
	return addr;
}

// get kernel pages after VM bootstraps
paddr_t
getkpages(unsigned long kpages)
{
	paddr_t addr;

	addr = coremap_kalloc(kpages);

	return addr;
}


/* Allocate/free some kernel-space pages */
paddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;

	if (vm_has_bootstrapped) {
		pa = getkpages(npages);
	} else {
		pa = getppages(npages);
	}

	if (pa==0)
		return 0;
	else
		return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
	if (vm_has_bootstrapped)
		coremap_kfree(addr);

	/* else { nothing - leak the memory.} */
}

///////////////////////////////
//// TLB STUFF/////////////////
///////////////////////////////

// handle page faults
int vm_fault(int faulttype, vaddr_t faultaddress) {
	
	(void)faulttype;
//	(void)faultaddress;

	uint32_t tlbhi;
	uint32_t tlblo;

	if (curthread->t_addrspace == NULL)	// kernel has page faulted, so return EFAULT, which will cause a panic (as it should)
		return EFAULT;

	faultaddress &= PAGE_FRAME;	// page-align the fault address

	
	struct page* pg = as_fault(curthread->t_addrspace,faultaddress);

	if (pg==NULL){
		return EFAULT;
	}
	
	spinlock_acquire(&pg->pg_lock);

		int stat = pg->status;

	spinlock_release(&pg->pg_lock);

	if (stat==NOT_ALLOCD) {
		int err = page_alloc(pg);
		if (err)			
			return err;
	}

	KASSERT((pg->ram_addr&PAGE_FRAME)==pg->ram_addr);
	KASSERT(pg->status==IN_MEM);

	spinlock_acquire(&pg->pg_lock);

		pg->is_dirty = 1;
		tlblo = (pg->ram_addr & TLBLO_PPAGE) | TLBLO_VALID | TLBLO_DIRTY;

	spinlock_release(&pg->pg_lock);

	tlbhi = faultaddress & TLBHI_VPAGE;

	spinlock_acquire(&tlb_lock);;	// only one thread should be messing with the TLB at a time

		//int probe = tlb_probe(tlbhi,0);
	
		//if (probe<0) 			
			tlb_random(tlbhi,tlblo);
		//else
		//	tlb_write(tlbhi,tlblo,probe);

		int probe = tlb_probe(tlbhi,0);

		KASSERT(probe>=0);

	spinlock_release(&tlb_lock);
		
	return 0;
}


void vm_tlbshootdown_all() {
	panic("We should not be doing a tlb_shootdown_all()!!!");
}


void vm_tlbshootdown(const struct tlbshootdown * t) {
	(void)t;
}

void vm_flushtlb(void) {
	int i;
	
	//int spl = splhigh();	// lock TLB while we mess with it
	spinlock_acquire(&tlb_lock);
	for (i=0; i<NUM_TLB;i++) {
		tlb_write(TLBHI_INVALID(i),TLBLO_INVALID(),i);	// invalidate all TLB entries
	}
	spinlock_release(&tlb_lock);
	//splx(spl);
}


////////////////////////////////////////////////////////////
////////////////// SWAPPING ////////////////////////////////
////////////////////////////////////////////////////////////

int swap_init(void) {
	int err = vfs_open((char*)"lhd1raw:",O_RDWR,0,&swapspace);	// swap to hard disk
	if (err)
		return err;

	swap_initialized = true;
	next_offset = 0;
	return 0;
}

int swap_in_page(struct page* p) {
	if (!swap_initialized) {
		int err = swap_init();
		if (err)
			return err;
	}

	spinlock_acquire(&p->pg_lock);

	paddr_t pa = coremap_ualloc(p);
	while (pa==0) {
		swap_out_page();
		pa = coremap_ualloc(p);
	}

	p->ram_addr = pa;
	read_in_page(p);
	p->status = IN_MEM;
	p->is_dirty = 0;
	
	spinlock_release(&p->pg_lock);
	return 0;
}

int swap_out_page(void) {
	if (!swap_initialized) {
		int err = swap_init();
		if (err)
			return err;
	}

	struct page* p = find_swapout();

	if (p==NULL)
		panic("Could not find a page to swap out");
	
	spinlock_acquire(&p->pg_lock);
	
	if (p->is_dirty)
		write_out_page(p);

	evict_page(p);
	p->status = IN_SWAP;
	p->ram_addr = 31;
	
	spinlock_release(&p->pg_lock);
	return 0;
}

int evict_page(struct page* p) {	// removes page from coremap
	if (!swap_initialized) {
		int err = swap_init();
		if (err) {
			return err;
		}
	}

	spinlock_acquire(&p->pg_lock);
	
	KASSERT(p->is_dirty==0);	// page should be clean
	KASSERT(p->status==IN_MEM);	// page should be in memory, but not tlb

	coremap_ufree(p->ram_addr);	// free the coremap entry
	
	spinlock_acquire(&p->pg_lock);

	return 0;
}

