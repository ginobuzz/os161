/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define ADDR_INLINE

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <addrspace.h>
#include <current.h>



struct addrspace* prev_as;

/* as_create - Creates an address space. Initializes its fields and returns it.
 * Returns NULL on out-of-memory error.
 */
struct addrspace*
as_create(void)
{
	struct addrspace* as;

	// Allocate space.
	as = kmalloc(sizeof(struct addrspace));
	if(as == NULL){
		return NULL;
	}

	// Create region linked list.
	as->vmregion = vm_region_create();
	if(as->vmregion == NULL){
		kfree(as);
		return NULL;
	}

	vm_region_init(as->vmregion);
	KASSERT(as->vmregion != NULL);

	return as;
}



 /* as_activate - make the specified address space the one currently "seen" by the processor. 
  * Argument might be NULL, meaning "no particular address space".
 */
void
as_activate(struct addrspace* as)
{
	if (as != prev_as) {
		prev_as = as;
		vm_flushtlb();
	}
}



int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace* new;
	int i;

	new = as_create();
	if (new == NULL) {
		return ENOMEM;
	}

	// Get size of region
	int num_regions = vm_region_num(old->vmregion);

	// Set size of new addrspace to match old's.
	vm_region_setsize(new->vmregion, num_regions);
	KASSERT((int)vm_region_num(new->vmregion) == num_regions);

	for(i = 0; i < num_regions; i++){
		struct region* src = vm_region_get(old->vmregion, i);
		KASSERT(src != NULL);
		vm_region_set(new->vmregion, i, region_copy(src));
	}

	*ret = new;

	return 0;
}



void
as_destroy(struct addrspace *as)
{
	KASSERT(as != curthread->t_addrspace);
	int i;

	// Get the size of the array.
	int size = vm_region_num(as->vmregion);

	// Destroy each VM Region in the array.
	for(i = 0; i < size; i++){

		// Get temporary reference to region.
		struct region* tmp = vm_region_get(as->vmregion, 0);

		// Destroy it!
		region_destroy(tmp);

		vm_region_remove(as->vmregion,0);
	}
	vm_region_cleanup(as->vmregion);
	vm_region_destroy(as->vmregion);
	as->heap = NULL;
	
	//KASSERT(as->heap==NULL);

	kfree(as);
	//vm_flushtlb();
}


int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, 
	int readable, int writeable, int executable)
{
	(void)executable;
	(void)writeable;
	(void)readable;

	size_t npages;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
	//kprintf("DEBUG[as_define_region]: requesting %d pages.\n", npages);
	int spl = splhigh();
	unsigned index;
	struct region* region;
	region = region_init(vaddr, npages);

	KASSERT(region_size(region) == (int)npages);

	vm_region_add(as->vmregion, region, &index);
	splx(spl);
	//kprintf("DEBUG[as_define_region]: creating region with base %x\n",region->base);

	return 0;
}


int
as_prepare_load(struct addrspace *as)
{	
	int reg_size;
	int size, i;
	vaddr_t heap_base = 0;

	// Get number of regions in current AS.
	size = vm_region_num(as->vmregion);

	// Find max base that isnt stack's.
	for(i = 0; i < size; i++)
	{
		struct region* tmp = vm_region_get(as->vmregion, i);
		vaddr_t base = tmp->base;

		if((base != USERSTACK) && (base > heap_base)) 
		{
			heap_base = tmp->base;
			reg_size = region_size(tmp);
		}
	}

	heap_base += reg_size * PAGE_SIZE;


	//kprintf("DEBUG[as_prepare_load]: creating heap region at %x.\n", heap_base);
	
	// Create heap region.
	as_define_region(as, heap_base, HEAP_PAGES * PAGE_SIZE, 0, 0, 0);


	// Get Heap reference.
	for(i = 0; i < size + 1; i++)
	{
		struct region* tmp = vm_region_get(as->vmregion, i);
		if(tmp->base == heap_base)
		{
			as->heap = vm_region_get(as->vmregion, i);
			as->heaptop = heap_base;
		}
	}


	return 0;
}


int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}


int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	*stackptr = USERSTACK;
	//kprintf("DEBUG[as_define_stack]: creating stack region.\n");
	as_define_region(as, USERSTACK, STACK_PAGES * PAGE_SIZE, 0, 0, 0);
	return 0;
}


struct page*
as_fault(struct addrspace* as, vaddr_t faultaddr) {

	int size, i;

	// Get number of regions to search through.
	size = vm_region_num(as->vmregion);

	// Search through all regions.
	for(i = 0; i < size; i++)
	{
		// Get temporary reference.
		struct region* target = vm_region_get(as->vmregion, i);

		// Get return value from region_fault.
		struct page* ret = region_fault(target, faultaddr);

		// Return if page found.
		if(ret != NULL)
		{
			return ret;
		}	
	}

	return NULL;
}
