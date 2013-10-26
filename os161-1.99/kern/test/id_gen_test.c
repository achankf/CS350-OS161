#include <id_generator.h>
#include <synch.h>
#include <lib.h>
#include <test.h>
#include <thread.h>

struct semaphore *sem_numthread;
struct id_generator *idgen;

static void test_generator(void *not_used, unsigned long threadid){

	(void) not_used;

	KASSERT(idgen != NULL);

	for (int i = 0; i < 100; i++){
		kprintf("%lu %d\n", threadid, idgen_get_next(idgen));
	}
	V(sem_numthread);
}

static void test_generator2(void *not_used, unsigned long threadid){

	(void) not_used;

	KASSERT(idgen != NULL);

	int32_t temp;
	for (int i = 0; i < 100; i++){
		temp = idgen_get_next(idgen);
		kprintf("%lu %d THEN PUT BACK\n", threadid, temp);
		idgen_put_back(idgen,temp);
	}
	V(sem_numthread);
}

int id_gen_test(int nargs, char **args){
	sem_numthread = sem_create("sem_testidgen",0);
	struct spinlock splock;
	spinlock_init(&splock);
	idgen = idgen_create(0);

	KASSERT(idgen != NULL);

	(void)nargs;
	(void)args;

	const int NUMTHREAD = 3;

	int i = 0;
	for (; i < NUMTHREAD; i++){
		kprintf ("MAKING THREAD %d\n", i);
		thread_fork("id_gen_test", NULL, test_generator, NULL, i);
	}

	for (int j = 0; j < NUMTHREAD; j++,i++){
		kprintf ("MAKING THREAD %d\n", i);
		thread_fork("id_gen_test", NULL, test_generator2, NULL, i);
	}

	for (int j = 0; j < 2*NUMTHREAD; j++){
		P(sem_numthread);
	}

	idgen_destroy(idgen);
	sem_destroy(sem_numthread);
	return 0;
}
