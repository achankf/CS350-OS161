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

#define SWAP_SIZE 2304 

struct swap_entry {
    bool part_of_pt;
    pid_t pid;
    int id;

};

// 9MB / PAGE_SIZE
struct swap_entry swaptable[SWAP_SIZE];
struct lock * swap_lock;
// the number of swap entries in side the swap table

struct id_generator*idgen;

void swaptable_init() {
    if (swap_lock == NULL) {
        swap_lock = lock_create("swap_lock");
    }
    bzero(swaptable, SWAP_SIZE);
    idgen = idgen_create(0);

}

int swap_to_disk (pid_t pid, int *id){
    (void) pid;
    (void) id;
#if 0 
    lock_acquire(swap_lock);

    // write swap entry to our swap file, but limit it to 9MB
    // 9 * 1024 * 1024 bytes
    struct vnode * swapfile;
    char *s = kstrdup("SWAPFILE");
    int err = vfs_open(s, O_RDWR|O_CREAT|O_TRUNC, 1, &swapfile);
    if (err != 0) {
        lock_release(swap_lock);    
        return err;
    }

    struct iovec iov;
    struct uio ku;

    // iovec, uio, buff, len, offset, UIO_WRITE 
    int index = idgen_get_next(idgen);
    int offset = PAGE_SIZE * index;
    //char * buf =  ; need to figure out the paddr
    uio_kinit(&iov,&ku, buf, PAGE_SIZE, offset,UIO_WRITE);
    err = VOP_WRITE(swapfile, &ku);

    if (err != 0) {
        lock_release(swap_lock);
        return err;
    }

    struct swap_entry entry;
    entry.id = id; 
    entry.pid = pid;
    entry.part_of_pt = false; // not sure

    swaptable[index] = entry;

    vfs_close(swapfile);
    lock_release(swap_lock);
#endif
    return 1;
}

int swap_to_mem (pid_t pid, int id){
    (void) pid;
    (void) id;

#if 0
    lock_acquire(swap_lock);

    struct vnode* swapfile;
    char *s = kstrdup("SWAPFILE");
    int err = vfs_open(s, O_RDWR|O_CREAT|O_TRUNC, 1, &swapfile);
    if (err != 0) {
        lock_release(swap_lock);
        return err;
    }

    int index=0;
        for (int i=0;i<SWAP_SIZE; i++) {
            if (swaptable[i]->pid == pid && swaptable[i]->id == id) {
                index = i;
                break;
            }
        }
    int offset = PAGE_SIZE * index;
    
    // find spot in memory to load the disk contents 
    
    
    
    
    
    idgen_put_back(idgen, index);
    vfs_close(swapfile);
    lock_release(swap_lock);
#endif
    return 1;

}
