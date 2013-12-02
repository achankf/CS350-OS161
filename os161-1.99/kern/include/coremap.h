#ifndef _COREMAP_H_
#define _COREMAP_H_

struct segment;
struct page_entry;

void coremap_init(void);
int kframe_alloc(int *frame, int frames_wanted);
int uframe_alloc1(int *frame, pid_t pid, int id); // alloc one frame
void frame_free(int frame);

// frame-to-kvaddr translation
paddr_t frame_to_paddr(int frame);
int kvaddr_to_frame(vaddr_t kvaddr);

// initialize structs that depend on kmalloc
void coremap_finalize(void);

// print the coremap by typing "cm" in the kernel
int coremap_show(int nargs, char **args);

// swapping-related functions
int get_page_entry_victim(struct page_entry **ret, int victim);
int coremap_has_space(void);
int coremap_get_rr_victim(void);
int core_kickvictim(int *victim);
#endif
