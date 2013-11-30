#include <segment.h>
#include <lib.h>
#include <kern/errno.h>
#include <vm.h>
#include <current.h>
#include <coremap.h>
#include <proc.h>

bool seg_is_inited(struct segment *seg){
	KASSERT(seg != NULL);
	return seg->vbase && seg->pagetable != NULL;
}

int seg_init(struct segment *seg, vaddr_t vbase, int npages){
	KASSERT(seg != NULL);

	seg->vbase = vbase & PAGE_FRAME;
	seg->npages = npages;
	seg->pagetable = kmalloc(npages * sizeof(struct page_entry));
	if (seg->pagetable == NULL) return ENOMEM;
	return 0;
}

void seg_cleanup(struct segment *seg){
	kfree(seg->pagetable);
}

int seg_translate(struct segment *seg, vaddr_t vaddr, paddr_t *ret){
	KASSERT(seg != NULL);
	KASSERT(seg_in_range(seg, vaddr));
	int vpn = ADDR_MAPPING_NUM(vaddr);
	int offset = ADDR_OFFSET(vaddr);
	int rv;

	int idx = vpn - seg->vbase;

	KASSERT(seg->pagetable[idx].alloc);

	if (seg->pagetable[idx].being_swapped){
		rv = swap_to_mem(curproc->pid, vpn);
		if (rv) {
			return rv;
		}
		seg->pagetable[idx].being_swapped = false; // in memory
	}

	*ret = (seg->pagetable[idx].pfn << 12) | offset;
	
	return 0;
}

bool seg_in_range(struct segment *seg, vaddr_t vaddr){
	KASSERT(seg != NULL);
	int vpn = ADDR_MAPPING_NUM(vaddr);
	int vpn_max = ADDR_MAPPING_NUM((seg->vbase + seg->npages * PAGE_SIZE));
	int vpn_min = ADDR_MAPPING_NUM(seg->vbase);
	return vpn >= vpn_min && vpn <= vpn_max;
}
