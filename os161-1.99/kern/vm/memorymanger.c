#include <synch.h>
// need to include coremap

struct memory_manager {
    spinlock *memlock;
    // base addr
    // last addr
    // coremap addr

};

struct memory_manager mem_man;

// functions
void man_init () {
    // initialize spinlock

    // set addresses

}

int swap_to_disk(pid_t p, int* page) {
    

}

int swap_to_mem(pid_t p, int* page) {
    

}
