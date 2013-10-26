#ifndef _ID_GENERATOR_H_
#define _ID_GENERATOR_H_

#include <spinlock.h>

struct id_generator;

/*
 * Memory management
*/
struct id_generator *idgen_create(int32_t start);
void idgen_destroy(struct id_generator *idgen);

/*
 * idgen_get_next - get the next available value from the generator
 * idgen_put_back - return an id to the generator
 */
int32_t idgen_get_next(struct id_generator *idgen);
int idgen_put_back(struct id_generator *idgen, int32_t val);
#endif
