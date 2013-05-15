#include <types.h>
#include <lib.h>
#include <mips/coremap.h>
#include <spinlock.h>
#include <mips/vm.h>
#include <vm.h>
#include <page.h>
#include <spl.h>


struct spinlock cm_lock;	// spinlock to protect coremap access
unsigned int coremap_size;	// size of coremap (should equal number of pages in RAM
paddr_t coremap;		// coremap starting address
struct cm_entry reserved_page;	// page reserved as temporary copy space
paddr_t ram_start;		// starting address of RAM
int free_mem_pages;		// free pages in RAM


void coremap_init(int num_pages, paddr_t mem_start, paddr_t cm_start) {

	int spl = splhigh();

	spinlock_init(&cm_lock);	// initialize coremap lock

	coremap_size = num_pages-1;
	ram_start = mem_start;
	coremap = cm_start;

	struct cm_entry* c = (struct cm_entry*)PADDR_TO_KVADDR(coremap);	// the initial coremap entry will be at the start address of the coremap
	int i;

	//spinlock_acquire(&cm_lock);
	for (i=0; i<num_pages; i++) {
		c[i].is_kern = 0;
		c[i].in_use = 0;
		c[i].pg = NULL;
		c[i].contig_pages = 0;
	}
	//spinlock_release(&cm_lock);

	free_mem_pages = num_pages-1;
	reserved_page = c[num_pages-1];

	splx(spl);
}

int coremap_ucount(void){
	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	int count = 0;

	int i;
	for (i=0; i<(int)coremap_size; i++) {
		if (cm_begin[i].in_use && !cm_begin[i].is_kern)count++;
	}

	return count;
}

int coremap_kcount(void){
	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	int count = 0;

	int i;
	for (i=0; i<(int)coremap_size; i++) {
		if (cm_begin[i].in_use && cm_begin[i].is_kern)count++;
	}

	return count;
}

void coremap_dump(void) {
	int spl = splhigh();
	kprintf("----COREMAP START----\n");

	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	int i;
	for (i=0; i<(int)coremap_size; i++) {
		if (cm_begin[i].in_use && cm_begin[i].is_kern)
			kprintf("index: %d, is_kern: %d, in_use: %d, page: %lx, contig_pages: %d\n",i,cm_begin[i].is_kern,cm_begin[i].in_use,(unsigned long)cm_begin[i].pg,cm_begin[i].contig_pages);
	}

	kprintf("----COREMAP END----\n");
	splx(spl);
}

void coremap_copy(paddr_t dest, paddr_t src) {
		spinlock_acquire(&cm_lock);
			memcpy((void*)PADDR_TO_KVADDR(dest),(void*)PADDR_TO_KVADDR(src),PAGE_SIZE);
		spinlock_release(&cm_lock);
}

paddr_t coremap_kalloc(unsigned long npages) {
	
	//kprintf("********************\nAllocating %x kpage(s)...\n",(int)npages);

	KASSERT((long)npages>0);

	paddr_t adr = 0;	// initial value, will be changed if page finding is successful

	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	spinlock_acquire(&cm_lock);	// lock the coremap

	//int before = coremap_kcount();

	if (free_mem_pages == 0)
			panic("We is out of pages :'-(...\n");	// don't need to release spinlock, as we are already screwed (for now)

	unsigned int i;
	for (i=0; i<=(coremap_size-npages); i++) {	// search for the first page in the contiguous allocation
		unsigned int j;
		bool success = 1;
		for (j=i; j<(i+npages); j++) {		// check the succeeding entries to see if they are free
			if (cm_begin[j].in_use) {
				success=0;		// one of the pages is being used, so we must start allocating from a different initial page
				i = j;			// set i to the page that was in use, so the next initial page is passed it (for efficiency)			
				break;
			}
		}
		if (success) {
			cm_begin[i].contig_pages=npages;	// mark how many pages we are allocating, used when freeing pages
			unsigned int k;
			for (k=i; k<(i+npages); k++) {
				cm_begin[k].is_kern = 1;	// kernel pages should not be swapped
				cm_begin[k].in_use = 1;
				cm_begin[k].pg = NULL;		// kernel pages do not map to page structs
			}
			adr = ram_start+(i*PAGE_SIZE);		// the physical address of the first page in this allocation
			free_mem_pages -= npages;
			break;

			KASSERT(cm_begin[i].contig_pages>0);
		}
	}

	//int after = coremap_kcount();

	//KASSERT(before+(int)npages == after);

	spinlock_release(&cm_lock);

	//KASSERT(adr>0);
	KASSERT(cm_begin[i].contig_pages>0);
	
	return adr;	// return the physical address of our allocation, or 0 for OUT-OF-MEMORY error
}

static
void coremap_clean_page(vaddr_t addr) {
	bzero((void*)addr,PAGE_SIZE);
	/*uint32_t *ptr = (void*)addr;
	size_t i;

	for (i=0; i<PAGE_SIZE/sizeof(uint32_t); i++) {
		ptr[i] = 0xbeefbeef;
	}
	*/	
}

void coremap_kfree(vaddr_t addr) {
	paddr_t a = KVADDR_TO_PADDR(addr);

	if (a<coremap) {
		return;	// this memory was allocated before the VM bootstrapped, so just leak it
	} else if (a>=coremap && a<ram_start) {
		panic("coremap_kfree tried to deallocate the coremap!!");
	}

	KASSERT((a&PAGE_FRAME)==a);

	int entry = (a-ram_start)/PAGE_SIZE;

	spinlock_acquire(&cm_lock);

	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	int pgs = cm_begin[entry].contig_pages;

	//int before = coremap_kcount();
	
	//kprintf("********************\nFreeing %x kpage(s)...\n",pgs);
	//coremap_dump();
	
	KASSERT(pgs>0);
	
	free_mem_pages += pgs;

	int i;
	for (i=0; i<pgs; i++) {	// mark all pages as not in use
		cm_begin[i+entry].in_use = 0;
		cm_begin[i+entry].is_kern = 0;
		cm_begin[i+entry].contig_pages = 0;
		coremap_clean_page(addr+(i*PAGE_SIZE));
	}

	//coremap_dump();

	//int after = coremap_kcount();

	//KASSERT(before-pgs == after);

	spinlock_release(&cm_lock);
}

// Allocate/free user-level page
paddr_t
coremap_ualloc(struct page* p) {
	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	spinlock_acquire(&cm_lock);

	//int before = coremap_ucount();

	int i;
	for(i=0;i<(int)coremap_size;i++) {
		if(cm_begin[i].in_use==0) {
			cm_begin[i].is_kern = 0;
			cm_begin[i].in_use = 1;
			cm_begin[i].pg = p;
			free_mem_pages--;
			spinlock_release(&cm_lock);
			return ram_start+(i*PAGE_SIZE);
		}
	}

	//int after = coremap_ucount();

	//KASSERT(before+1 == after);

	spinlock_release(&cm_lock);
	return 0;	// no swapping yet, so just return 0 for failure
}

void
coremap_ufree(paddr_t page_addr) {
	KASSERT(page_addr!=31);
	int entry = (page_addr-ram_start)/PAGE_SIZE;
	struct cm_entry* cm_begin = (struct cm_entry*)PADDR_TO_KVADDR(coremap);
	
	spinlock_acquire(&cm_lock);

	//int before = coremap_ucount();

	cm_begin[entry].in_use = 0;
	cm_begin[entry].pg = NULL;
	coremap_clean_page(PADDR_TO_KVADDR(page_addr));
	free_mem_pages++;

	//int after = coremap_ucount();
	
	//KASSERT(before-1 == after);

	spinlock_release(&cm_lock);
}

struct page*
coremap_find_swapout(void) {
/*
	KASSERT(free_mem_pages==0);

	struct page* p;
	int rating = 4;

	struct cm_entry* c = (struct cm_entry*)PADDR_TO_KVADDR(coremap);

	int i;
	spinlock_acquire(&cm_lock);
	for (i=0;i<(int)coremap_size;i++) {

		KASSERT(c[i].in_use==1);	// should not be here if we have unused physical pages

		if (c[i].is_kern)
			continue;

		spinlock_acquire(&c[i].pg->pg_lock);

		if (c[i].pg->status==NOW_SWAPPING) {
			spinlock_release(&c[i].pg->pg_lock);
			continue;
		}

		if (!c[i].pg->is_dirty && !c[i].pg->status==IN_TLB) {
			p = c[i].pg;
			spinlock_release(&c[i].pg->pg_lock);
			break;
		} else if (!c[i].pg->is_dirty && c[i].pg->status==IN_TLB) {
			if (rating>1) {
				p = c[i].pg;
				rating = 1;
			}
		} else if (c[i].pg->is_dirty && !c[i].pg->status==IN_TLB) {
			if (rating>2) {
				p = c[i].pg;
				rating = 2;
			}
		} else {
			p = c[i].pg;
			rating = 3;
		}
		
		spinlock_release(&c[i].pg->pg_lock);
	}
	
	spinlock_acquire(&p->pg_lock);
	p->status = NOW_SWAPPING;
	spinlock_release(&p->pg_lock);

	spinlock_release(&cm_lock);
	return p;
*/
	return NULL;
}

