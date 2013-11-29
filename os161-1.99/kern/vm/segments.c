#include <segment.h>
#include <lib.h>
#include <kern/errno.h>

bool seg_is_inited(struct segment *seg){
	return seg->vbase;
}

int seg_init(struct segment *seg, int vbase, int npages){
	seg->pagetable = kmalloc(npages * sizeof(struct page_entry));
	if (seg->pagetable == NULL) return ENOMEM;
	seg->vbase = vbase;
	seg->npages = npages;
	return 0;
}

void seg_cleanup(struct segment *seg){
	kfree(seg->pagetable);
}
