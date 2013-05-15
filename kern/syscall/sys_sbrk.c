#include <syscall.h>




int
sys_sbrk(intptr_t amount, int32_t* retval) {
/* INPUT:
 *  amount - # of bytes needed to be allocated on heap.
 *  reval  - return value.
 */	
	*retval = curthread->t_addrspace->heaptop;
	
	//struct 	 region* heap;
	int 	 bytes;

	// Cast input to int, for safty.
	bytes = (int) amount;
	if (bytes%2 != 0)
		return EINVAL;

	int region_top = region_gettop(curthread->t_addrspace->heap);	// last address that is currently allocated


	// Check if user wants to alloc or dealloc.
	if (bytes == 0) {
		return 0;
	} else if (bytes > 0) {
		int count = 0;
		int regtop_tmp = region_top;
		while ((curthread->t_addrspace->heaptop + bytes) > regtop_tmp) {
			if (regtop_tmp >= (int)curthread->t_addrspace->heap->base+(HEAP_PAGES*PAGE_SIZE)) return ENOMEM;	// we will exceed out heap limit, so fail
			
			regtop_tmp += PAGE_SIZE;	// increment our temporary region top
			count++;			// increment the count of the number of pages we will have to add
		}

		int err = region_alloc(curthread->t_addrspace->heap,count);	// allocate the new pages to the heap
		if (err==ENOMEM) {
			return ENOMEM;
		}
		

	} else if (bytes < 0) {
		if (region_top == (int)curthread->t_addrspace->heap->base) {
				return EINVAL;
		}

		int count = 0;
		int regtop_tmp = region_top;

		while ((curthread->t_addrspace->heaptop - bytes) < (regtop_tmp-PAGE_SIZE)) {
			regtop_tmp -= PAGE_SIZE;
			count++;

			if (regtop_tmp == (int)curthread->t_addrspace->heap->base) return EINVAL;
		}

		region_dealloc(curthread->t_addrspace->heap,count);

	} else {
		panic("sbrk() is breaking the laws of mathematics...\n");
	}
		

	curthread->t_addrspace->heaptop += bytes;	
		
	return 0;
}
