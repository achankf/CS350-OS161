#ifndef _ID_GENERATOR_H_
#define _ID_GENERATOR_H_

#include <spinlock.h>

struct id_generator;

/*
 * Memory management
*/
struct id_generator *idgen_create(uint32_t start);
void idgen_destroy(struct id_generator *idgen);

/*
 * idgen_get_next - get the next available value from the generator
 * idgen_put_back - return an id to the generator
 */
void idgen_get_next(struct id_generator *idgen, struct spinlock *spinlock, uint32_t *ret);
int idgen_put_back(struct id_generator *idgen, struct spinlock *spinlock, uint32_t val);
#endif
