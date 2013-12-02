#include <types.h>
#include <coremap.h>
#include <vnode.h>
#include <segment.h>
#include <synch.h>
#include <kern/fcntl.h>
#include <addrspace.h>
#include <vm.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <swapfile.h>
#include <current.h>
#include <proc.h>
#include <uw-vmstats.h>

#define SWAP_FILE_SIZE_IN_MB (9)
#define SWAP_SIZE (SWAP_FILE_SIZE_IN_MB * (1 << 20) / PAGE_SIZE)

struct swap_entry {
	pid_t pid;
	bool used;
};

// 9MB / PAGE_SIZE
struct swap_entry swaptable[SWAP_SIZE];
struct lock * swap_lock;
// the number of swap entries in side the swap table

struct vnode *swapfile;


static void swaptable_init() {
	swap_lock = lock_create("swap_lock");
	KASSERT(swap_lock != NULL);
	bzero(swaptable, SWAP_SIZE * sizeof(struct swap_entry));
}

int swap_sweep(pid_t pid){
	int count = 0;
	lock_acquire(swap_lock);
		for (int i = 0; i < SWAP_SIZE; i++){
			if (swaptable[i].pid != pid) continue;
			bzero(&swaptable[i], sizeof(struct swap_entry));
			count++;
		}
	lock_release(swap_lock);
	return count;
}

int swapfile_init()
{
	swaptable_init();
	char *swapfile_path = kstrdup("SWAPFILE");
	int result = vfs_open(swapfile_path, O_RDWR|O_CREAT|O_TRUNC, 0, &swapfile);
	kfree(swapfile_path);        
	return result;
}

void swapfile_close()
{
	vfs_close(swapfile);
}


int swap_to_disk (struct page_entry *pe)
{
	struct iovec iov;
	struct uio ku;
	int offset;
	int result = 0;
	paddr_t pa;

	lock_acquire(swap_lock);
	for(int i = 0; i < SWAP_SIZE; i++)
	{
		if(!swaptable[i].used)
		{
			offset = i * PAGE_SIZE;
			swaptable[i].pid = curproc->pid;
			swaptable[i].used = true;
			pe->swap_index = i;
			goto SWAP_TO_DISK_NOT_FULL;
		}
	}

	panic("The swap file is full\n");

SWAP_TO_DISK_NOT_FULL:

	pa = PADDR_TO_KVADDR(frame_to_paddr(pe->pfn));
kprintf("\n%x writting %p into swap\n", pe->pfn, (void*)pa);
	char dummy[PAGE_SIZE];
	bzero(dummy, PAGE_SIZE);

	uio_kinit(&iov, &ku, dummy, PAGE_SIZE, offset, UIO_WRITE);
	uio_kinit(&iov, &ku, (void *)pa, PAGE_SIZE, offset, UIO_WRITE);
	result = VOP_WRITE(swapfile, &ku);
	DEBUG(DB_VM,"swapped index %d to disk.\n", pe->swap_index);
	if(result) goto END_OF_SWAP_TO_DISK;
	pe->swapped = true;
	vmstats_inc(9);

END_OF_SWAP_TO_DISK:
	lock_release(swap_lock);
	return result;
}

int swap_to_mem (struct page_entry *pe, int apfn)
{
	struct iovec iov;
	struct uio ku;

	lock_acquire(swap_lock);
	pe->swapped = false;
	int offset = pe->swap_index * PAGE_SIZE;
	paddr_t pa = PADDR_TO_KVADDR(frame_to_paddr(apfn));
	bzero((void*)pa, PAGE_SIZE);

	uio_kinit(&iov, &ku, (void *)pa, PAGE_SIZE, offset, UIO_READ);
	int result = VOP_READ(swapfile, &ku);

	if(result) goto END_OF_SWAP_TO_MEM;

	pe->pfn = apfn;
	swaptable[pe->swap_index].used = false;
	vmstats_inc(6); // Page Faults (Disk)
	vmstats_inc(8); // Page Faults from Swapfile
END_OF_SWAP_TO_MEM:
	lock_release(swap_lock);
 	return result;
}
