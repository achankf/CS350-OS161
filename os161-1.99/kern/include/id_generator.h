#ifndef _ID_GENERATOR_H_
#define _ID_GENERATOR_H_

#include <synch.h>

struct id_generator;

/*
	Get the next availble value 
*/

struct id_generator *idgen_create(uint32_t start);

void idgen_get_next(struct id_generator *idgen, struct lock *lock, uint32_t *ret);

int idgen_put_back(struct id_generator *idgen, struct lock *lock, uint32_t val);
 
int idgen_recycle(struct id_generator *idgen, struct lock *lock);

void idgen_destroy(struct id_generator *idgen);
#endif
