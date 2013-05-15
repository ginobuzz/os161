#define REGION_INLINE
#include <vm_region.h>
#include <addrspace.h>


struct region*
region_init(vaddr_t base, int npages)
{
	//kprintf("[region_init]->Base: %lx, Pages: %d\n",(unsigned long)base,npages);
	struct region* region;
	unsigned index;
	int i;

	/* Allocate memory for region. */
	region = kmalloc(sizeof(struct region));
	if(region == NULL) return NULL;

	region->pagetable = pagetable_create();
	if(region->pagetable == NULL){
		kfree(region);
		return NULL;
	}

	/* Set region's base. */
	region->base = base;

	/* Initialize array. */
	pagetable_init(region->pagetable);

	/* Initialize lock. */
	region->lock = lock_create("Region Lock");

	/* Add virtual pages to regions pagetable. */
	for(i = 0; i < npages; i++)
			pagetable_add(region->pagetable, page_create(), &index);
	

	return region;
}


struct region*
region_copy(struct region* src)
{
	struct region* dest;
	int i;

	int size = region_size(src);

	// Initialize destiniation region.
	dest = region_init(src->base, size);


	lock_acquire(dest->lock);

	// Copy over pages.
	for(i = 0; i < size; i++){
		struct page* page = pagetable_get(src->pagetable, i);
		struct page* new_page = page_copy(page);
		pagetable_set(dest->pagetable, i, new_page);
	}

	lock_release(dest->lock);

	return dest;
}


int 
region_size(struct region* region)
{
	lock_acquire(region->lock);
	int size = (int) pagetable_num(region->pagetable);
	lock_release(region->lock);
	return size;
} 


void
region_remove(struct region* region, int index)
{
	lock_acquire(region->lock);
	pagetable_remove(region->pagetable, index);
	lock_release(region->lock);
}


void
region_destroy(struct region* region)
{
	
	//kprintf("[region_destroy]->Base: %lx, Pages: %d\n",(unsigned long)region->base,pagetable_num(region->pagetable));
	int size, i;

	lock_acquire(region->lock);

	// Get size of pagetable.
	size = pagetable_num(region->pagetable);

	if (region->base == USERSTACK)
		KASSERT(size==64);

	// Go through the page table and destroy each page.
	for(i = 0; i < size; i++){

		// Make temporary reference to the page.
		struct page* tmp = pagetable_get(region->pagetable, 0);

		// Remove it from array.
		pagetable_remove(region->pagetable, 0);

		// Destroy it!
		page_destroy(tmp);
		kfree(tmp);
		//KASSERT((tmp->ram_addr==0xdeadbeef) || (tmp->ram_addr==0x0));
		tmp = NULL;
	}

	KASSERT(pagetable_num(region->pagetable) == 0);

	pagetable_cleanup(region->pagetable);
	// Destroy the array.
	pagetable_destroy(region->pagetable);

	lock_release(region->lock);
	lock_destroy(region->lock);

	kfree(region);

}


/* Determines if the faultaddress belongs to the target region's 
 * page table. If it does, it will return it. returns NULL if it 
 * is unsucessful. */
struct page*
region_fault(struct region* target, vaddr_t faddr)
{
	vaddr_t base, last;
	int size, index;

	lock_acquire(target->lock);
	base = target->base;
	size = pagetable_num(target->pagetable);
	last = base + ((size-1) * PAGE_SIZE);

	// If its the stack-region, addresses grow backwards.
	if(base == USERSTACK)
	{
		last = base - ((size-1) * PAGE_SIZE);
		if( (faddr >= last) && (faddr <= base) )
		{
			index = (base - faddr)/PAGE_SIZE;
			lock_release(target->lock);
			return pagetable_get(target->pagetable, index); 
		}
	}

	// If fault address is between base and last, page is in this region.
	else if( (faddr >= base) && (faddr <= last) )
	{
		index = (faddr - base)/PAGE_SIZE;
		lock_release(target->lock);
		return pagetable_get(target->pagetable, index); 
	}

	lock_release(target->lock);
	return NULL;
}


vaddr_t
region_alloc(struct region* region, int npages)
{
	lock_acquire(region->lock);
	
	vaddr_t top = region->base;
	int count = 0, pages = 0;
	int err, i;
	int size = pagetable_num(region->pagetable);// Size of pagetable.

	// Confirm we have 'npages' consecutive pages.
	for(i = 0; i < size; i++)
	{
		struct page* page = pagetable_get(region->pagetable, i);
		if(page->status == (int)NOT_ALLOCD) ++count; 
		else count = 0;
	}
	if(npages > count)
	{
		lock_release(region->lock);
		return ENOMEM;// NOT ENOUGH FREE PAGES.
	}


	// Allocate pages.
	for(i = 0; i < size; i++){
		struct page* page = pagetable_get(region->pagetable, i);
		if(page->status == (int)NOT_ALLOCD)
		{
			if(top == 0)
			{
				top = region->base + (i * PAGE_SIZE);
				KASSERT((top & PAGE_FRAME) == top);
			}

			if(pages <= npages)
			{
				err = page_alloc(page);
				if(err){
					lock_release(region->lock);
					return err;
				}
				++pages;
			}
		}
	}
	KASSERT((top & PAGE_FRAME)==top);
	lock_release(region->lock);
	return top;
}


vaddr_t
region_dealloc(struct region* region, int npages)
{
	lock_acquire(region->lock);
	
	vaddr_t top = region->base;
	int count = 0, pages = 0;
	int size, i;

	// Get size of pagetable.
	size = pagetable_num(region->pagetable);

	// Confirm we have 'npages' consecutive pages allocated.
	for(i = 0; i < size; i++)
	{
		struct page* page = pagetable_get(region->pagetable, i);
		if(page->status != (int)NOT_ALLOCD) ++count; 
	}
	if(npages > count){
		lock_release(region->lock);
		return EINVAL;// NOT ENOUGH ALLOCATED PAGES.
	}

	// Deallocate pages.
	for(i = size - 1; i >= 0; i--){
		struct page* page = pagetable_get(region->pagetable, i);
		if(page->status != (int)NOT_ALLOCD)
		{
			if(pages <= npages)
			{
				page_free(page);
				++pages;
			}
		}
	}
	KASSERT((top & PAGE_FRAME)==top);
	lock_release(region->lock);
	return top;
}

vaddr_t
region_gettop(struct region* region){
	lock_acquire(region->lock);
	
	int count = 0;
	int i = 0;

	vaddr_t top = region->base;
	KASSERT((top & PAGE_FRAME)==top);


	struct page* p = pagetable_get(region->pagetable, i);
	int size = pagetable_num(region->pagetable);


	while((p->status != NOT_ALLOCD)&&(count < size)){
		++count;
		if (count>=HEAP_PAGES) break;
		p = pagetable_get(region->pagetable, count);
	}


	top += count * PAGE_SIZE;
	KASSERT((top & PAGE_FRAME)==top);

	lock_release(region->lock);
	return top;
}
