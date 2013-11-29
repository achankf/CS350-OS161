typedef enum {STOLEN, KERNEL, USER, UNALLOCATED} core_status_t;
typedef enum {IN_MEM, SWAPPED, UNINIT} page_status_t;
struct coremap_entry
{
	core_status_t status;
	segment* seg_ptr;
	int offset;
};

struct pagetable_entry
{
	page_status_t status;
	bool read_only;
	int frame;
};

struct segment
{
	int vaddr_start;
	int size;
	struct pagetable_entry *contents;
};

struct coremap
{
	struct coremap_entry *coremap_ptr;
	int size;
};


int kframe_alloc(int *frame)
{
	for(int i = 0; i < cm.size; i++)
	{
		if(cm.coremap_ptr[i].status == UNALLOCATED)
		{
			cm.coremap_ptr[i].status = KERNEL;
			cm.coremap_ptr[i].seg_ptr = NULL;
			cm.coremap_ptr[i].offset = -1;
			*frame = i;
			return 0;
		}
	}
	return 1;
}

int frame_alloc(segment* user_seg, int *frame)
{
	for(int i = 0; i < cm.size; i++)
	{
		if(cm.coremap_ptr[i].status == UNALLOCATED)
		{
			cm.coremap_ptr[i].status = USER;
			cm.coremap_ptr[i].seg_ptr = user_seg;
			cm.coremap_ptr[i].offset = i - user_seg.vaddr_start; 
			frame = &i;
			return 0;
		}
	}
	return 1;
}
	
void frame_free(int frame)
{
	cm.coremap_ptr[frame].status = UNALLOCATED;
	cm.coremap_ptr[frame].seg_ptr = NULL;
	cm.coremap_ptr[frame].offset = -1;
}


