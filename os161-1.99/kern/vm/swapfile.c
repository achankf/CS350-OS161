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
#include <id_generator.h>
#include <coremap.h>
#include <uw-vmstats.h>

#define SWAP_FILE_SIZE_IN_MB (9)
#define SWAP_SIZE (SWAP_FILE_SIZE_IN_MB * (1 << 20) / PAGE_SIZE)

struct swap_entry {
	bool part_of_pt;
	pid_t pid;
	int id;
	bool used;
};

// 9MB / PAGE_SIZE
struct swap_entry swaptable[SWAP_SIZE];
struct lock * swap_lock;
// the number of swap entries in side the swap table

struct id_generator*idgen;
struct vnode *swapfile;


static void swaptable_init() {
    if (swap_lock == NULL) {
        swap_lock = lock_create("swap_lock");
    }
    bzero(swaptable, SWAP_SIZE * sizeof(struct swap_entry));
    idgen = idgen_create(0);
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
	lock_acquire(swap_lock);
	pe->being_swapped = true;
	paddr_t pa = pe->pfn * PAGE_SIZE;
	struct iovec iov;
	struct uio ku;
	int offset;
	bool full = true;
	int result;
	for(int i = 0; i < SWAP_SIZE; i++)
	{
		if(!swaptable[i].used)
		{
			offset = i * PAGE_SIZE;
			full = false;
			swaptable[i].used = true;
			pe->swap_index = i;
			break;
		}
	}
	if(full)
	{
		lock_release(swap_lock);
		panic("The swap file is full");
	}

	uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(pa), PAGE_SIZE, offset, UIO_WRITE);
	result = VOP_WRITE(swapfile, &ku);
	DEBUG(DB_VM,"swapped index %d to disk.\n", pe->swap_index);
	if(result) goto END_OF_SWAP_TO_DISK;
	vmstats_inc(9);

END_OF_SWAP_TO_DISK:
	lock_release(swap_lock);
	return result;
}

int swap_to_mem (struct page_entry *pe, int apfn)
{
	lock_acquire(swap_lock);
	pe->being_swapped = false;
	struct iovec iov;
	struct uio ku;
	int offset = pe->swap_index * PAGE_SIZE;
	paddr_t pa = apfn * PAGE_SIZE;
	uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(pa), PAGE_SIZE, offset, UIO_READ);
	int result = VOP_READ(swapfile, &ku);

	if(result)
	{
		lock_release(swap_lock);
		return result;
	}

	pe->pfn = apfn;
	swaptable[pe->swap_index].used = false;
	DEBUG(DB_VM,"swapped back to memory %d\n", pe->swap_index);
	lock_release(swap_lock);
	vmstats_inc(8);
	return 0;
}
