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
#include <swapfile.h>
#include <proc.h>
#include <segment.h>

#define ZERO_OUT_FRAME(frame) (bzero((void*)PADDR_TO_KVADDR(frame_to_paddr(frame)), PAGE_SIZE))

int num_frames;
paddr_t base;
struct lock *coremap_lock = NULL;
struct coremap_entry *coremap_ptr = NULL;
bool booting = true;
struct queue *id_not_used;

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
	num_frames = (hi-lo) / PAGE_SIZE; 

	/*
	 * find out the block of memory required for the coremap
	 * rounded to the next page size
	 */
	int coremap_size = num_frames * sizeof(struct coremap_entry);
	int coremap_end_size = coremap_size / PAGE_SIZE + 1;
	coremap_ptr = (struct coremap_entry*) PADDR_TO_KVADDR(ram_stealmem(coremap_end_size));

	// finalize the memory pool
	ram_getsize(&lo, &hi);
	base = lo;
	coremap_size = num_frames * sizeof(struct coremap_entry);
	num_frames = (hi - lo) / PAGE_SIZE;

	bzero(coremap_ptr, coremap_size);
}

int
uframe_alloc1(int *frame, pid_t pid, int id)
{
	int ret = 1;
	lock_acquire(coremap_lock);
	if(coremap_has_space() == 0)
	{
		ret = core_kickvictim(frame);
		if(ret) goto UFRAME_ALLOC1_FINISH;
		set_frame(*frame, USER, pid, id);
		ret = 0;
		goto UFRAME_ALLOC1_FINISH;
	}

	
	// start search from the MIDDLE of the coremap
	int idx = num_frames >> 1;
	for(int i = 0; i < num_frames; i++) {
		if(coremap_ptr[idx].status == UNALLOCATED) {
			set_frame(idx, USER, pid, id);
			*frame = idx;
			ret = 0;
			ZERO_OUT_FRAME(*frame);
			break;
		}
		idx = (idx + 1) % num_frames;
	}
	UFRAME_ALLOC1_FINISH:
	lock_release(coremap_lock);

	DEBUG(DB_VM,"Finished uframe_alloc1 retval:%d\n", ret);
	return ret;
}

void frame_free(int frame)
{
	KASSERT(frame >= 0 && frame < num_frames);

	lock_acquire(coremap_lock);
		int pid = coremap_ptr[frame].pid;
		int id = coremap_ptr[frame].id;

		if (pid != 0) {
			KASSERT(curproc->pid == pid);
			set_frame(frame, UNALLOCATED, 0,0);
			goto FRAME_FREE_DONE;
		}
		for (int i = 0; i < num_frames; i++){
			if (coremap_ptr[i].pid != 0 || coremap_ptr[i].id != id) continue;
			set_frame(i, UNALLOCATED, 0,0);
			if (id_not_used != NULL){ // recycle id's for kernel memory
				q_addtail(id_not_used, (void*)id);
			}
			goto FRAME_FREE_DONE;
		}

		panic("Invalid freeing of the frame for the unique pair (pid,id)=(%d,%d)\n", pid, id);
FRAME_FREE_DONE:
	lock_release(coremap_lock);
}

int kframe_alloc(int *frame, int frames_wanted)
{

	int rv, id;
	static int id_cur = 0;

	// only acquire lock and initialize idgen AFTER booting -- due to kmalloc
	if (booting){
		id = id_cur++;
	} else {
		lock_acquire(coremap_lock);
			if (!q_empty(id_not_used)){
				id = (int) q_remhead(id_not_used);
			} else {
				id = id_cur++;
			}
	}
        int need_to_free = frames_wanted - coremap_has_space();
        if(need_to_free > 0)
        {
                for(int i = 0; i < need_to_free; i++)
                {
                        int victim = coremap_get_rr_victim();
                        struct page_entry *pe;
                        int result = get_page_entry_victim(&pe, victim);
                        if(result)
                        {
                                lock_release(coremap_lock);
                                return result;
                        }
                        swap_to_disk(pe);
                }
        }

	rv = frame_alloc_continuous(frame, KERNEL, 0, id, frames_wanted);
	if (id_not_used != NULL) {
		q_addtail(id_not_used, (void*)id);
	}
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
	coremap_lock = lock_create("coremap lock");
	id_not_used = q_create(num_frames);

	if (coremap_lock == NULL || id_not_used == NULL){
		panic("Not enough memory for the base system!!!!\n");
	}
	booting = false;
}

int coremap_show(int nargs, char **args){
	(void) nargs;
	(void) args;
	kprintf("---------------- Coremap ----------------\n");
	int b = 0;
	for (int i = 0; i < num_frames; i++){
		if (b == 0) kprintf("\n| ");
		kprintf("%3d:%3d (%3d,%3x) | ", i, coremap_ptr[i].status, coremap_ptr[i].pid, coremap_ptr[i].id);
		b = (b+1) % 6;
	}
	kprintf("\n");
	return 0;
}

int coremap_has_space()
{
	int used_frame = 0;
	for(int i = 0; i < num_frames; i++)
	{
		if(coremap_ptr[i].status != UNALLOCATED)
			used_frame++;
	}
	return num_frames - used_frame;
}
	
int coremap_get_rr_victim()
{
        int victim;
        static unsigned int next_victim = 0;

	bool full = true;
        while(true)
        {
                if(coremap_ptr[next_victim].status == USER)
                {
			victim = next_victim;
			coremap_ptr[next_victim].status = UNALLOCATED;
			coremap_ptr[next_victim].pid = 0;
			coremap_ptr[next_victim].id = 0;
                        next_victim = (next_victim + 1) % num_frames;
			full = false;
			break;
		}
		next_victim = (next_victim + 1) % num_frames;
	}
	
	if(full)
		panic("cannot find victim to swap\n");
		
	return victim;
}

int get_page_entry_victim(struct page_entry **ret, int victim)
{	
	struct proc *victim_proc;
	struct lock *v_lock = proctable_lock_get();
	lock_acquire(v_lock);
	victim_proc = proc_getby_pid(coremap_ptr[victim].pid);
	
	struct segment *victim_seg;
	
	int result = as_which_seg(victim_proc->p_addrspace, (coremap_ptr[victim].id) << 12, &victim_seg);
	if(result) goto ENDOF_GET_VICTIM;
	int vpn = coremap_ptr[victim].id - ADDR_MAPPING_NUM(victim_seg->vbase);
	*ret = &victim_seg->pagetable[vpn];

ENDOF_GET_VICTIM:
	lock_release(v_lock);
	return result;
}

int core_kickvictim(int *ret)
{
	int victim = coremap_get_rr_victim();
	struct page_entry *pe;
	//kprintf("full, victim is %d\n", victim);
	int result = get_page_entry_victim(&pe, victim);
	//kprintf("back from page_entry %p, result = %d\n", pe, result);
	if(result)
	{
		return result;
	}
	swap_to_disk(pe);
	*ret = victim;
	return 0;
}
