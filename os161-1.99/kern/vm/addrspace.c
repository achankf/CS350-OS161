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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <segment.h>
#include <spl.h>
#include <mips/tlb.h>
#include <elf.h>
#include <vm.h>
#include <coremap.h>
#include <vfs.h>
#include <current.h>
#ifdef UW
#include <proc.h>
#endif

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	bzero(as, sizeof(struct addrspace));
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	(void)old;
	(void)ret;

	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

#if 0
	newas->as_vbase1 = old->as_vbase1;
	newas->as_npages1 = old->as_npages1;
	newas->as_vbase2 = old->as_vbase2;
	newas->as_npages2 = old->as_npages2;

	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(newas)) {
		as_destroy(newas);
		return ENOMEM;
	}

	KASSERT(newas->as_pbase1 != 0);
	KASSERT(newas->as_pbase2 != 0);
	KASSERT(newas->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(newas->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(newas->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(newas->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	*ret = newas;
#endif
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	for (int i = 0; i < NUM_SEGS; i++){
		seg_cleanup(&as->segs[i]);
	}
	core_sweep(curproc->pid);
	vfs_close(as->v);
	kfree(as);
}

void
as_activate(bool flushtlb)
{
	int spl;
	struct addrspace *as;

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	if (flushtlb){
		for (int i=0; i<NUM_TLB; i++) {
			tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
		}
	}

	splx(spl);
}

void
#ifdef UW
as_deactivate(void)
#else
as_dectivate(void)
#endif
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 
	DEBUG(DB_VM, "\tRegion: p_vaddr:%p, R:%d W:%d X:%d\n", (void*)vaddr, readable, writeable, executable);

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	for (int i = 0; i < NUM_SEGS - 1; i++){ // -1 because stack is separate
		if (seg_is_inited(&as->segs[i])) continue;
		return seg_init(&as->segs[i], as, vaddr & PAGE_FRAME, npages, readable, writeable, executable);
	}

	/*
	 * Support for more than two regions is not available.
	 */

	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	as->segs[STACK].ondemand = false;
	bzero(&(as->segs[STACK].ph), sizeof(Elf_Phdr));

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK - PAGE_SIZE;
	DEBUG(DB_VM,"define stack: %p\n", (void*)*stackptr);
	return seg_init(&as->segs[STACK], as, USERSTACK - PAGE_SIZE * (DUMBVM_STACKPAGES + 1), DUMBVM_STACKPAGES, true, true, false);
	
}

int as_which_seg(struct addrspace *as, vaddr_t vaddr, struct segment **ret){
	for (int j = 0; j < NUM_SEGS; j++){
		KASSERT(seg_is_inited(&as->segs[j]));
		DEBUG(DB_VM,"Checking vaddr %p, seg:%p, segment base:%p, segment size:%d\n", (void*)vaddr, &as->segs[j], (void*)as->segs[j].vbase, as->segs[j].npages);
		if (!seg_in_range(&as->segs[j], vaddr)) continue;
		*ret = &as->segs[j];
		return 0;
	}
	return EFAULT;
}
