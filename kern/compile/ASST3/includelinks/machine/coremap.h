#ifndef _MIPS_COREMAP_H_
#define _MIPS_COREMAP_H_

struct cm_entry {
	unsigned is_kern:1;	// kernel pages cannot be swapped
	unsigned in_use:1;	// whether the page is being used or not
	struct page* pg;	// pointer to the process's page struct, NULL for kernel pages
	int contig_pages;	// number of continuous pages allocated at once (only for kfree)
};

void coremap_init(int num_pages, paddr_t ram_start, paddr_t cm_start);

// used to find/free KERNEL pages only!!!!
paddr_t coremap_kalloc(unsigned long npages);
void coremap_kfree(vaddr_t addr);

// used to find/free USER pages
paddr_t coremap_ualloc(struct page* p);
void coremap_ufree(paddr_t page_addr);

struct page* coremap_find_swapout(void);
void coremap_copy(paddr_t dest, paddr_t src);

#endif
