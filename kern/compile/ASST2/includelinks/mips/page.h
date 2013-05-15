#ifndef _MIPS_PAGE_H_
#define _MIPS_PAGE_H_

struct vnode* swapspace;
bool swap_initialized;
off_t next_offset;

int write_out_page(struct page* p);
int read_in_page(struct page* p);
struct page* find_swapout(void);



#endif

