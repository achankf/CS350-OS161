
#ifndef _COREMAP_H_
#define _COREMAP_H_

struct segment;

void coremap_init(void);
int kframe_alloc(int *frame, int id, int frames_wanted);
//int uframe_alloc(int *frame, pid_t pid, int id, int frames_wanted);
int uframe_alloc1(int *frame, pid_t pid, int id);
void frame_free(int frame);
paddr_t frame_to_paddr(int frame);
int kvaddr_to_frame(vaddr_t kvaddr);


void coremap_finalize(void);

int coremap_show(int nargs, char **args);
#endif
