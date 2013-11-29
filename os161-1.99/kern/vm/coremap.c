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
int coremap_frames;
struct coremap_entry *coremap_ptr;


static void set_frame(core_status_t c_status, struct segment *ptr, int c_vp_num, int frame)
{
        coremap_ptr[frame].status = c_status;
        coremap_ptr[frame].seg_ptr = ptr;
        coremap_ptr[frame].vp_num = c_vp_num;
}

int coremap_init()
{
	paddr_t lo,hi;
	ram_getsize(&lo, &hi);
	ram_size = (hi-lo);
	num_of_frames = ram_size / PAGE_SIZE; 
	coremap_size = num_of_frames * sizeof(struct coremap_entry);
	coremap_frames = ((coremap_size / PAGE_SIZE) + 1);
	coremap_ptr = (struct coremap_entry*)lo;

	for(int i = 0; i < coremap_frames; i++)
	{
		set_frame(STOLEN, NULL, -1, i);
	}
	return 0;
}
	

int kframe_alloc(int *frame)
{
	for(int i = 0; i < num_of_frames; i++)
	{
		if(coremap_ptr[i].status == UNALLOCATED)
		{
			set_frame(KERNEL, NULL, -1, i);
			*frame = i;
			return 0;
		}
	}
	return 1;
}

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
	
void frame_free(int frame)
{
	set_frame(UNALLOCATED, NULL, -1, frame);
}
