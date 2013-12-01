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
#include <coremap.h>
#include <synch.h>

#define ZERO_OUT_FRAME(frame) (bzero((void*)PADDR_TO_KVADDR(frame_to_paddr(frame)), PAGE_SIZE))

int num_frames;
paddr_t base;
struct coremap_entry *coremap_ptr;
struct lock *coremap_lock = 0;

bool booting = true;

typedef enum {
	UNALLOCATED, USER, KERNEL
} core_status_t;

struct coremap_entry{
	core_status_t status;
	pid_t pid;
	int id;
};

static void
set_frame(int frame, core_status_t status, pid_t pid, int id) {
	coremap_ptr[frame].status = status;
	coremap_ptr[frame].pid = pid;
	coremap_ptr[frame].id = id;
}

static int core_find_next(int idx, int *ret){
	//kprintf("%p %d %p %d\n", coremap_ptr, idx, ret, num_frames);
	for (int i = idx; i < num_frames; i++){
		if(coremap_ptr[i].status != UNALLOCATED) continue;
		*ret = i;
		return 0;
	}
	return ENOMEM;
}

static void
frame_range_mark(int start, int size, core_status_t status, pid_t pid, int id){
	int idx = start;
	for (int i = 0; i < size; i++, idx++)
		set_frame(idx, status, pid, id);
}

static int
frame_find_continuous(int *retframe, int frames_wanted){
	int prev;
	int rv = core_find_next(0, &prev);

	if (rv) return rv;

	int i;
	for (i = prev; i < num_frames; i++){
			if(coremap_ptr[i].status == UNALLOCATED) continue;
			if (i - prev >= frames_wanted){
				break;
			}
			rv = core_find_next(i, &prev);
			if (rv) return rv;
			i = prev;
	}

	if (i - prev >= frames_wanted){
		*retframe = prev;
		return 0;
	}

	return ENOMEM;
}

static int
frame_alloc_continuous(int *frame, core_status_t status, pid_t pid, int id, int frames_wanted)
{
	int rv = frame_find_continuous(frame, frames_wanted);
	if (rv) return rv;
	frame_range_mark(*frame, frames_wanted, status, pid, id);
	return 0;
}

void
coremap_init()
{
	paddr_t lo,hi;
	ram_getsize(&lo, &hi);
	int ram_size = (hi-lo);
	int num_of_frames = ram_size / PAGE_SIZE; 

	/*
	 * find out the block of memory required for the coremap
	 * rounded to the next page size
	 */
	int coremap_size = num_of_frames * sizeof(struct coremap_entry);
	int coremap_end_size = coremap_size / PAGE_SIZE + 1;
	coremap_ptr = (struct coremap_entry*) PADDR_TO_KVADDR(ram_stealmem(coremap_end_size));
	ram_getsize(&lo, &hi);

	// finalize the memory pool
	base = lo;
	num_frames = (hi - lo) / PAGE_SIZE;
	coremap_size = num_of_frames * sizeof(struct coremap_entry);

	bzero(coremap_ptr, coremap_size);

	// variable booting avoid lock_acquire in kframe_alloc
	coremap_lock = lock_create("coremap lock");
}

int
uframe_alloc1(int *frame, pid_t pid, int id)
{
	int ret = 1;

	DEBUG(DB_VM,"Running uframe_alloc1\n");
	lock_acquire(coremap_lock);
		for(int i = 0; i < num_frames; i++) {
			if(coremap_ptr[i].status == UNALLOCATED) {
				set_frame(i, USER, pid, id);
				*frame = i;
				ret = 0;
				ZERO_OUT_FRAME(*frame);
				break;
			}
		}
	lock_release(coremap_lock);

	DEBUG(DB_VM,"Finished uframe_alloc1 retval:%d\n", ret);
	return ret;
}

void frame_free(int frame)
{
	lock_acquire(coremap_lock);
		set_frame(frame, UNALLOCATED, 0,0);
	lock_release(coremap_lock);
}

int kframe_alloc(int *frame, int id, int frames_wanted)
{
	int rv;

	if (!booting) lock_acquire(coremap_lock);
		rv = frame_alloc_continuous(frame, KERNEL, 0, id, frames_wanted);
	if (!booting) lock_release(coremap_lock);
	return rv;
}

paddr_t frame_to_paddr(int frame){
	return base + (frame << 12);
}

int kvaddr_to_frame(vaddr_t kvaddr){
	return (kvaddr - 0x80000000 - base) >> 12;
}

void coremap_finalize(void){
	booting = false;
}

int coremap_show(int nargs, char **args){
	(void) nargs;
	(void) args;
	kprintf("---------------- Coremap ----------------\n");
	int b = 0;
	for (int i = 0; i < num_frames; i++){
		kprintf("%3d:%3d %3d %3d    ", i, coremap_ptr[i].status, coremap_ptr[i].pid, coremap_ptr[i].id);
		b = (b+1) % 6;
		if (b == 0) kprintf("\n");
	}
	kprintf("\n");
	return 0;
}
