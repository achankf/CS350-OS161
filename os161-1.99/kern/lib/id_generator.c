#include <id_generator.h>
#include <queue.h>
#include <lib.h>

#define INIT_Q_SIZE 10

struct id_generator{
	uint32_t current;
	struct queue *not_used;
};

struct id_generator *idgen_create(uint32_t start){
	struct id_generator *idgen = kmalloc(sizeof(struct id_generator));
	idgen->current = start;
	idgen->not_used = q_create(INIT_Q_SIZE);
	return idgen;
}

void idgen_get_next(struct id_generator *idgen, struct spinlock *spinlock, uint32_t *ret){
	KASSERT(idgen != NULL);
	KASSERT(spinlock != NULL);
	KASSERT(ret != NULL);
	KASSERT(idgen->not_used != NULL);
	KASSERT(spinlock_do_i_hold(spinlock));
	if (!q_empty(idgen->not_used)){
		*ret = (uint32_t)q_remhead(idgen->not_used);
	} else {
		*ret = idgen->current;
		idgen->current++;
	}
}

int idgen_put_back(struct id_generator *idgen, struct spinlock *spinlock, uint32_t val){
	KASSERT(idgen != NULL);
	KASSERT(spinlock != NULL);
	KASSERT(idgen->not_used != NULL);
	KASSERT(spinlock_do_i_hold(spinlock));

	return q_addtail(idgen->not_used, (void*) val);
}

void idgen_destroy(struct id_generator *idgen){
	KASSERT(idgen != NULL);
	while(!q_empty(idgen->not_used)){
		q_remhead(idgen->not_used);
	}
	q_destroy(idgen->not_used);
}
