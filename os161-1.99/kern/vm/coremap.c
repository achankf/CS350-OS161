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

int num_frames;
struct coremap_entry *coremap_ptr;
struct spinlock coremap_lock = SPINLOCK_INITIALIZER;

typedef enum {
	UNALLOCATED, USER, KERNEL
} core_status_t;

struct coremap_entry{
	core_status_t status;
	pid_t pid;
	int vpn;
};

static void
set_frame(int frame, core_status_t status, pid_t pid, int vpn)
{
	// must hold the spinlock
	KASSERT(spinlock_do_i_hold(&coremap_lock));
	coremap_ptr[frame].status = status;
	coremap_ptr[frame].pid = pid;
	coremap_ptr[frame].vpn = vpn;
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
	coremap_ptr = (struct coremap_entry*) ram_stealmem(coremap_end_size);
	ram_getsize(&lo, &hi);
	num_frames = (hi - lo) / PAGE_SIZE;
}
	

static int
frame_alloc(int *frame, core_status_t status, pid_t pid, int vpn)
{
	int ret = 1;
	
	spinlock_acquire(&coremap_lock);
		// pick a random location and then do linear search
		int idx = random();
		for(int i = 0; i < num_frames; i++) {
			idx = (idx + 1) % num_frames;
			if(coremap_ptr[i].status == UNALLOCATED) {
				set_frame(i, status, pid, vpn);
				*frame = i;
				ret = 0;
				break;
			}
		}
	spinlock_release(&coremap_lock);
	return ret;
}

int kframe_alloc(int *frame, int vpn)
{
	return frame_alloc(frame, KERNEL, 0, vpn);
}

void frame_free(int frame)
{
	spinlock_acquire(&coremap_lock);
		set_frame(frame, UNALLOCATED, 0,0);
	spinlock_release(&coremap_lock);
}
