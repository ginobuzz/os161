#ifndef _MIPS_MIVM_H
#define _MIPS_MIVM_H

#include <types.h>
#include <mips/vm.h>
#include <page.h>

paddr_t getkpages(unsigned long kpages);
paddr_t getppages(unsigned long npages);

int swap_init(void);
int swap_in_page(struct page* p);
int swap_out_page(void);
int evict_page(struct page* p);


#endif
