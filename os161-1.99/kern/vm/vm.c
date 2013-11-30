
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
	paddr_t paddr;
	bool dirty;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "vm_fault: 0x%x, type:%d\n", faultaddress, faulttype);

	switch (faulttype) {
		// handle the READ ONLY segment case
		case VM_FAULT_READONLY:
			kprintf("Attempt to write to READ ONLY segment\n");
			sys__exit(1);
			panic("The zombie walks!");
		case VM_FAULT_READ:
			DEBUG(DB_VM,"\t vm_fault: READ\n");
			break;
		case VM_FAULT_WRITE:
			DEBUG(DB_VM,"\t vm_fault: WRITE\n");
			break;
		default:
			return EINVAL;
	}

	if (curproc == NULL) {
		DEBUG(DB_VM, "\tcannot get current process\n");
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		DEBUG(DB_VM, "\tcannot get address space\n");
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	// check the consistency of the address space and find the physical address
	for (int j = 0; j < NUM_SEGS; j++){
		KASSERT(seg_is_inited(&as->segs[j]));
		if (!seg_in_range(&as->segs[j], faultaddress)) continue;
		if (seg_translate(&as->segs[j], faultaddress, &paddr)){
			panic("Cannot translate a valid vaddr\n");
		}
		dirty = as->segs[j].writeable;
		goto VM_FAULT_VALID_ADDRESS;
	}

	DEBUG(DB_VM, "\tvaddr not in range\n");
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

dirty= true;
	ehi = faultaddress;
	elo = paddr | (dirty ? TLBLO_DIRTY : 0) | TLBLO_VALID;

	DEBUG(DB_VM, "\tTo be written to TLB: fault 0x%x -> paddr 0x%x; ehi:%x, elo:%x\n", faultaddress, paddr, ehi, elo);
	tlb_write(ehi, elo, i);
	splx(spl);
	return 0;	
}
#endif /* OPT_VM */

#endif /* UW */
