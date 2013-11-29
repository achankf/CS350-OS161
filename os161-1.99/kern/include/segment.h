#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>

enum {
	TEXT, DATA, STACK, NUM_SEGS
};

struct page_entry{
	int vpn;
	bool swap;
};

struct segment{
	int vbase;
	int npages;
	struct page_entry *pagetable;
};

bool seg_is_inited(struct segment *seg);
int seg_init(struct segment *seg, int vbase, int npages);
void seg_cleanup(struct segment *seg);

#endif
