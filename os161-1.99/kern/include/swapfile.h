#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

struct swap_entry;

void swaptable_init(void);
int swap_to_disk(pid_t,int*);
int swap_to_mem(pid_t, int);


#endif
