#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>

enum {
	TEXT, DATA, STACK, NUM_SEGS
};

struct page_entry{
	int pfn;
	bool being_swapped;
	bool alloc;
};

struct segment{
	vaddr_t vbase;
	int npages;
	struct page_entry *pagetable;
	struct addrspace *as;
	bool readable, writeable, executable;
};

bool seg_is_inited(struct segment *seg);
int seg_init(struct segment *seg, struct addrspace *as, vaddr_t vbase, int npages, bool R, bool W, bool X);
void seg_cleanup(struct segment *seg);
int seg_translate(struct segment *seg, vaddr_t vaddr, paddr_t *ret);
bool seg_in_range(struct segment *seg, vaddr_t vaddr);

#endif
