#include <types.h>
#include <coremap.h>
#include <vfs.h>
#include <vnode.h>
#include <synch.h>

struct vnode *swapfile;

struct swap_entry {
    bool part_of_pt;
    pid_t pid;
    int id;
    char block[4096]; // data in memory
};

int swap_to_disk (pid_t pid, int *frame){
	(void)pid;
	(void)frame;
    
#if 0
   struct vnode* swapfile;
   int err = vfs_open("lhd0raw:", O_RDWR, 0, &swapfile);
   if (err != 0) {
           return err;
   }

    

#endif
    return 1;
}

int swap_to_mem (pid_t pid, int vpn){

    struct vnode* swapfile;
	(void) pid;
	(void) vpn;
	(void) swapfile;

    return 1;

}
