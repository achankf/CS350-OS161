
#ifndef _COREMAP_H_
#define _COREMAP_H_

struct segment;

int coremap_init(void);
int kframe_alloc(int *frame, int vpn);
//int frame_alloc(struct segment *user_seg, int *frame);
void frame_free(int frame);

#endif
