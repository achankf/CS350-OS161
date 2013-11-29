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

int coremap_size;
int ram_size;
int num_of_frames;
struct coremap_entry *coremap_ptr;
struct spinlock coremap_lock;
paddr_t lo,hi;

typedef enum {
	UNALLOCATED, USER, STOLEN, KERNEL
} core_status_t;

struct coremap_entry{
	core_status_t status;
	pid_t pid;
	int vpn;
};

#if 0
static void set_frame(core_status_t c_status, struct segment *ptr, int c_vp_num, int frame)
{
        coremap_ptr[frame].status = c_status;
        coremap_ptr[frame].seg_ptr = ptr;
        coremap_ptr[frame].vp_num = c_vp_num;
}
#endif

static void
set_frame(int frame, core_status_t status, pid_t pid, int vpn)
{
        coremap_ptr[frame].status = status;
        coremap_ptr[frame].pid = pid;
        coremap_ptr[frame].vpn = vpn;
}

int coremap_init()
{
	ram_getsize(&lo, &hi);
	ram_size = (hi-lo);
	num_of_frames = ram_size / PAGE_SIZE; 

	/*
	 * find out the block of memory required for the coremap
	 * rounded to the next page size
	 */
	coremap_size = num_of_frames * sizeof(struct coremap_entry);
	coremap_size += PAGE_SIZE - (coremap_size % PAGE_SIZE);
	KASSERT(coremap_size % PAGE_SIZE);

	coremap_ptr = (struct coremap_entry*)lo;

	// get ``rid" of the coremap from visisble memory
	lo += coremap_size;

	// recalculate the size of the memory
	ram_size = (hi-lo);
	num_of_frames = ram_size / PAGE_SIZE; 

	return 0;
}
	

static int
frame_alloc(int *frame, core_status_t status, pid_t pid, int vpn)
{
	int ret = 1;
	
	for(int i = 0; i < num_of_frames; i++)
	{
		if(coremap_ptr[i].status == UNALLOCATED)
		{
			set_frame(i, status, pid, vpn);
			*frame = i;
			ret = 0;
		}
	}
	return ret;
}

int kframe_alloc(int *frame, int vpn)
{
	return frame_alloc(frame, KERNEL, 1, vpn);
}

#if 0
int frame_alloc(struct segment *user_seg, int *frame)
{
	for(int i = 0; i < num_of_frames; i++)
	{
		if(coremap_ptr[i].status == UNALLOCATED)
		{
			set_frame(USER, user_seg, (i - user_seg->vaddr_start), i);
			frame = &i;
			return 0;
		}
	}
	return 1;
}
#endif
	
void frame_free(int frame)
{
	(void) frame;
	//set_frame(UNALLOCATED, NULL, -1, frame);
}
