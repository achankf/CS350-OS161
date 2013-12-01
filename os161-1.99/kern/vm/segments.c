#include <segment.h>
#include <lib.h>
#include <kern/errno.h>
#include <vm.h>
#include <vnode.h>
#include <current.h>
#include <uio.h>
#include <coremap.h>
#include <proc.h>
#include <swapfile.h>
#include <elf.h>
#include <addrspace.h>

static
int
load_segment(struct addrspace *as, struct vnode *v,
	     off_t offset, vaddr_t vaddr, 
	     size_t memsize, size_t filesize,
	     int is_executable)
{
	KASSERT(vaddr < 0x80000000);
	DEBUG(DB_VM, "Running load segment %p, size %d\n", (void*)vaddr, memsize);

	struct iovec iov;
	struct uio u;
	int result;

	if (filesize > memsize) {
		DEBUG(DB_VM,"ELF: warning: segment filesize > segment memsize\n");
		filesize = memsize;
	}

	DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n", (unsigned long) filesize, (unsigned long) vaddr);

	struct segment *seg;
	paddr_t kpaddr;
	result = as_which_seg(as, vaddr, &seg);
	if (result) return result;
	result = seg_translate(seg, vaddr, &kpaddr);
	if (result) return result;

	(void) is_executable;
	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(kpaddr);
	iov.iov_len = memsize;		 // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = filesize;          // amount to read from the file
	u.uio_offset = offset;
	u.uio_segflg = UIO_SYSSPACE;// is_executable ? UIO_USERISPACE : UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = NULL;

	DEBUG(DB_VM,"ELF loading segment into %p(%p) as %p\n", (void*)PADDR_TO_KVADDR(kpaddr), (void*) kpaddr,(void*)vaddr);

	result = VOP_READ(v, &u);
	if (result) {
		return result;
	}

	DEBUG(DB_VM, "\tDone loading\n");

	return result;
}

bool seg_is_inited(struct segment *seg){
	KASSERT(seg != NULL);
	return seg->vbase && seg->pagetable != NULL && seg->as != NULL;
}

int seg_init(struct segment *seg, struct addrspace *as, vaddr_t vbase, int npages, bool R, bool W, bool X){
	KASSERT(seg != NULL);
	KASSERT(as != NULL);

	seg->pagetable = kmalloc(npages * sizeof(struct page_entry));
	if (seg->pagetable == NULL) return ENOMEM;

	seg->vbase = vbase & PAGE_FRAME;
	seg->npages = npages;
	seg->as = as;
	seg->readable = R;
	seg->writeable = W;
	seg->executable = X;
	return 0;
}

void seg_cleanup(struct segment *seg){
	if (seg == NULL) return;

	for (int i = 0; i < seg->npages; i++){
		if (!seg->pagetable[i].alloc) continue;
		DEBUG(DB_VM,"Deallocating pfn:%d for process %d's page %d\n", seg->pagetable[i].pfn, curproc->pid, i);
		frame_free(seg->pagetable[i].pfn);
	}
	kfree(seg->pagetable);
}

int seg_ondemand_load(struct segment *seg, int idx){
	DEBUG(DB_VM,"On-demanding page loading on index %d\n", idx);
	int result = uframe_alloc1(&seg->pagetable[idx].pfn, curproc->pid, idx+ADDR_MAPPING_NUM(seg->vbase));
	if (result) return result;
	seg->pagetable[idx].alloc = true;

	DEBUG(DB_VM,"\tDone allocation, segment %p loading on demand? %d\n", seg, seg->ondemand);

	if (!seg->ondemand) return 0; // let process to write

	int idx_offset = idx * PAGE_SIZE;
	int fsize = seg->ph.p_filesz - idx_offset;
	DEBUG(DB_VM,"\tProcess to be read vnode %p, offset:%x, size:%d\n", seg->as->v, idx_offset, fsize);
	if (fsize <= 0) return 0;

	// text/data segment -- load on demand
	result = load_segment(seg->as,
		seg->as->v,
		seg->ph.p_offset + idx_offset,
		seg->vbase + idx_offset,
		PAGE_SIZE,
		fsize,
		seg->ph.p_flags & PF_X);
	if (result) return result;
	return 0;
}

int seg_translate(struct segment *seg, vaddr_t vaddr, paddr_t *ret){
	KASSERT(seg != NULL);
	KASSERT(seg_in_range(seg, vaddr));
	KASSERT(seg_is_inited(seg));

	DEBUG(DB_VM,"seg_translate %p\n", (void*)vaddr);

	int vpn = ADDR_MAPPING_NUM(vaddr);
	int offset = ADDR_OFFSET(vaddr);
	int rv;

	int idx = vpn - ADDR_MAPPING_NUM(seg->vbase);

	DEBUG(DB_VM,"\tindex values %x, vpn %x, vbase %x\n", idx, vpn,ADDR_MAPPING_NUM(seg->vbase));

	if(!seg->pagetable[idx].alloc){
		rv = seg_ondemand_load(seg, idx);
		if (rv){
			return rv;
		}
		DEBUG(DB_VM,"\tFrame %d allocated for vpn %x (index:%d)\n", seg->pagetable[idx].pfn, vpn, idx);
	}

	if (seg->pagetable[idx].being_swapped){
		DEBUG(DB_VM,"Swapping in %d\n", idx);
		int frame;
		int result = uframe_alloc1(&frame, curproc->pid, vpn);
		if(result)
		{
			result = core_kickvictim(&frame);	
			if (result) return result;
		}
		seg->pagetable[idx].pfn = frame;
		rv = swap_to_mem(&seg->pagetable[idx], frame); // swap back
		if (rv) {
			return rv;
		}
	}

	*ret = frame_to_paddr(seg->pagetable[idx].pfn) | offset;
	DEBUG(DB_VM,"\tfinish v->p addr translation: %p\n", (void*) *ret);
	
	return 0;
}

bool seg_in_range(struct segment *seg, vaddr_t vaddr){
	KASSERT(seg != NULL);
	int vpn = ADDR_MAPPING_NUM(vaddr);
	int vpn_max = ADDR_MAPPING_NUM((seg->vbase + seg->npages * PAGE_SIZE));
	int vpn_min = ADDR_MAPPING_NUM(seg->vbase);
	return vpn >= vpn_min && vpn < vpn_max;
}
