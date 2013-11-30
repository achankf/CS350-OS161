
#ifdef UW
/* This was added just to see if we can get things to compile properly and
 * to provide a bit of guidance for assignment 3 */

#include "opt-vm.h"
#if OPT_VM

#include <types.h>
#include <lib.h>
#include <spl.h>
#include <mips/tlb.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <current.h>
#include <spinlock.h>
#include <uw-vmstats.h>
#include <coremap.h>
#include <syscall.h>

void
vm_bootstrap(void)
{
	/* May need to add code. */
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	int frame;
	int rv;
	rv = kframe_alloc(&frame, 0, npages);

	DEBUG(DB_VM, "Getting %x\n", frame_to_paddr(frame));

	if (rv > 0) return 0;
	return PADDR_TO_KVADDR(frame_to_paddr(frame));
}

void 
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */
	DEBUG(DB_VM,"freeing %p\n", (void*) addr);

	(void)addr;
}

void
vm_tlbshootdown_all(void)
{
	panic("Not implemented yet.\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("Not implemented yet.\n");
}

static int tlb_get_rr_victim()
{
	int victim;
	static unsigned int next_victim = 0;
	victim = next_victim;
	next_victim = (next_victim + 1) % NUM_TLB;
	return victim;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    
	//vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	bool dirty;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;

	faultaddress &= PAGE_FRAME;
	dirty = true;

	DEBUG(DB_VM, "smartbvm: fault: 0x%x, type:%d\n", faultaddress, faulttype);

	switch (faulttype) {
        // handle the READ ONLY segment case
        case VM_FAULT_READONLY:
            kprintf("Attempt to write to READ ONLY segment");
            sys__exit(1);

        case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	// check the consistency of the address space and find the physical address
	for (int i = 0; i < NUM_SEGS; i++){
		KASSERT(seg_is_inited(&as->segs[i]));
		if (!seg_in_range(&as->segs[i], faultaddress)) continue;
		if (seg_translate(&as->segs[i], faultaddress, &paddr)){
			panic("Cannot translate a valid vaddr\n");
		}
		if (i == TEXT) dirty = false;
		goto VM_FAULT_VALID_ADDRESS;
	}

	return EFAULT;

VM_FAULT_VALID_ADDRESS:

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	
	i = tlb_get_rr_victim();
	tlb_read(&ehi, &elo, i);
	if (elo & TLBLO_VALID) {
		DEBUG(DB_VM, "Overwritting a valid entry in TLB.\n");
	}
	ehi = faultaddress;
	//elo = paddr | (dirty? TLBLO_DIRTY : 0) | TLBLO_VALID;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	tlb_write(ehi, elo, i);
	splx(spl);
	return 0;	
}
#endif /* OPT_VM */

#endif /* UW */
