#include <id_generator.h>
#include <synch.h>
#include <lib.h>
#include <test.h>
#include <thread.h>

struct temp{
	struct lock *testlock;
	struct id_generator *idgen;
};

struct semaphore *sem_numthread;
struct temp t;

static void test_generator(void *not_used, unsigned long threadid){

	(void) not_used;

	KASSERT(t.idgen != NULL);
	KASSERT(t.testlock != NULL);

	uint32_t temp;
	for (int i = 0; i < 100; i++){
		lock_acquire(t.testlock);
			idgen_get_next(t.idgen,t.testlock,&temp);
		lock_release(t.testlock);
		kprintf("%lu %d\n", threadid, temp);
	}
	V(sem_numthread);
}

static void test_generator2(void *not_used, unsigned long threadid){

	(void) not_used;

	KASSERT(t.idgen != NULL);
	KASSERT(t.testlock != NULL);

	uint32_t temp;
	for (int i = 0; i < 100; i++){
		lock_acquire(t.testlock);
			idgen_get_next(t.idgen,t.testlock,&temp);
			idgen_put_back(t.idgen,t.testlock,temp);
		lock_release(t.testlock);
		kprintf("%lu %d THEN PUT BACK\n", threadid, temp);
	}
	V(sem_numthread);
}

int id_gen_test(int nargs, char **args){
	sem_numthread = sem_create("sem_testidgen",0);
	t.testlock = lock_create("id_gen_testlock");
	t.idgen = idgen_create(0);

	KASSERT(t.idgen != NULL);
	KASSERT(t.testlock != NULL);

	(void)nargs;
	(void)args;

	const int NUMTHREAD = 3;

	int i = 0;
	for (; i < NUMTHREAD; i++){
		kprintf ("MAKING THREAD %p %d\n", t.idgen, i);
		thread_fork("id_gen_test", NULL, test_generator, NULL, i);
	}

	for (int j = 0; j < NUMTHREAD; j++,i++){
		kprintf ("MAKING THREAD %p %d\n", t.idgen, i);
		thread_fork("id_gen_test", NULL, test_generator2, NULL, i);
	}

	for (int j = 0; j < 2*NUMTHREAD; j++){
		P(sem_numthread);
	}

	lock_destroy(t.testlock);
	idgen_destroy(t.idgen);
	sem_destroy(sem_numthread);
	return 0;
}
