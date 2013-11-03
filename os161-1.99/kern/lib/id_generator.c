#include <id_generator.h>
#include <queue.h>
#include <lib.h>
#include <spinlock.h>

#define INIT_Q_SIZE 10

struct id_generator{
	int32_t current;
	struct queue *not_used;
	struct spinlock lock;
};

struct id_generator *idgen_create(int32_t start){
	struct id_generator *idgen = kmalloc(sizeof(struct id_generator));
	if (idgen == NULL){
		return NULL;
	}
	spinlock_init(&idgen->lock);
	idgen->current = start;
	idgen->not_used = q_create(INIT_Q_SIZE);
	return idgen;
}

int32_t idgen_get_next(struct id_generator *idgen){
	KASSERT(idgen != NULL);
	KASSERT(idgen->not_used != NULL);

	int32_t ret;
	spinlock_acquire(&idgen->lock);
		if (!q_empty(idgen->not_used)){
			ret = (int32_t)q_remhead(idgen->not_used);
		} else {
			ret = idgen->current;
			idgen->current++;
		}
	spinlock_release(&idgen->lock);
	return ret;
}

int idgen_put_back(struct id_generator *idgen, int32_t val){
	KASSERT(idgen != NULL);
	KASSERT(idgen->not_used != NULL);

	int ret = 0;
	spinlock_acquire(&idgen->lock);
		ret = q_addtail(idgen->not_used, (void*) val);
	spinlock_release(&idgen->lock);
	return ret;
}

void idgen_destroy(struct id_generator *idgen){
	KASSERT(idgen != NULL);
	spinlock_cleanup(&idgen->lock);
	while(!q_empty(idgen->not_used)){
		q_remhead(idgen->not_used);
	}
	q_destroy(idgen->not_used);
	kfree(idgen);
}

bool idgen_reach(struct id_generator *idgen, int32_t val){
	bool ret;

	KASSERT(idgen != NULL);

	spinlock_acquire(&idgen->lock);
		ret = q_empty(idgen->not_used) && idgen->current >= val;
	spinlock_release(&idgen->lock);
	return ret;
}
