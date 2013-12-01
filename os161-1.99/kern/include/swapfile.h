#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

struct swap_entry;

void swaptable_init(void);
int swapfile_init(void);
int swap_to_disk(struct page_entry *pe);
int swap_to_mem(struct page_entry *pe, int apfn);

#endif
