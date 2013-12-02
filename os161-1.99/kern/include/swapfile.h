#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>

struct swap_entry;

int swapfile_init(void);
int swap_to_disk(struct page_entry *pe);
int swap_to_mem(struct page_entry *pe, int apfn);
void swapfile_close(void);
int swap_sweep(pid_t pid);
#endif
