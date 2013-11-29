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

struct coremap

int frame_alloc(int *frame)
{
	for(int i = 0; i < size; i++)
	{
		if(cm.coremap_ptr[i].status == UNALLOCATED)
		{
			cm.coremap_ptr[i].status = USER;
			segemtn
	




