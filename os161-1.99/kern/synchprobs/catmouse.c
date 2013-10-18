/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 * 05-01-2012: TBB : added comments to try to clarify use/non use of volatile
 * 22-08-2013: TBB: made cat and mouse eating and sleeping time optional parameters
 *
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <synchprobs.h>

/*
 * 
 * cat,mouse,bowl simulation functions defined in bowls.c
 *
 * For Assignment 1, you should use these functions to
 *  make your cats and mice eat from the bowls.
 * 
 * You may *not* modify these functions in any way.
 * They are implemented in a separate file (bowls.c) to remind
 * you that you should not change them.
 *
 * For information about the behaviour and return values
 *  of these functions, see bowls.c
 *
 */

/* this must be called before any calls to cat_eat or mouse_eat */
extern int initialize_bowls(unsigned int bowlcount);

extern void cleanup_bowls( void );
extern void cat_eat(unsigned int bowlnumber, int eat_time );
extern void mouse_eat(unsigned int bowlnumber, int eat_time);
extern void cat_sleep(int sleep_time);
extern void mouse_sleep(int sleep_time);

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver
 *  function, catmouse(), based on the problem parameters
 *  that are passed in from the kernel menu command or
 *  kernel command line.
 *
 * Once they have been set, you probably shouldn't be
 *  changing them.
 *
 * These are only ever modified by one thread, at creation time,
 * so they do not need to be volatile.
 */
int NumBowls;  // number of food bowls
int NumCats;   // number of cats
int NumMice;   // number of mice
int NumLoops;  // number of times each cat and mouse should eat


static volatile int num_free_bowls;
static volatile int cats_waiting;
static volatile int mice_waiting;
static volatile int eating_cats_count;
static volatile int eating_mice_count;

// 0 for free, 1 for cat, 2 for mouse
static volatile int * bowl_status; // array of ints to track bowls

static struct lock *bowl_lock; // access bowls
static struct cv *cat_cv;   // track cats at bowl
static struct cv *free_cv;  // track free bowls


/* 
 * Defaults here are as they were with the previos implementation
 * where these could not be changed.
 */
int CatEatTime = 1;      // length of time a cat spends eating
int CatSleepTime = 5;    // length of time a cat spends sleeping
int MouseEatTime = 3;    // length of time a mouse spends eating
int MouseSleepTime = 3;  // length of time a mouse spends sleeping

/*
 * Once the main driver function (catmouse()) has created the cat and mouse
 * simulation threads, it uses this semaphore to block until all of the
 * cat and mouse simulations are finished.
 */
struct semaphore *CatMouseWait;

/*
 * 
 * Function Definitions
 * 
 */


/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 */

// sets up some varibales
static void init(void) {
  num_free_bowls= NumBowls;

  if (bowl_lock==NULL) {
    bowl_lock = lock_create("bowl_lock");
  }
  if (free_cv==NULL) {
    free_cv = cv_create("free_cv");
  }
  if (cat_cv==NULL) {
	  cat_cv = cv_create("cat_cv");
  }
  if (bowl_status==NULL) {
	  bowl_status = kmalloc(NumBowls * sizeof(int));

    for (int i=0; i<NumBowls; i++) {
      bowl_status[i] = 0; // set all to none
    }
  }
}

// returns the first available empty bowl returns 0 if none avail
// valid bowls are 1 to NumBowls
static int get_free_bowl() {
  if (num_free_bowls==0) {
    return 0;
  }
  for (int i=0; i<NumBowls; i++) {
    if (bowl_status[i]==0) {
      return i+1;
    }
  }
  return 0; // in case we can't find a bowl
}

static bool if_eating(int creature) {
  for(int i=0; i<NumBowls; i++) {
    if (bowl_status[i]==creature) {
      return true;
    }
  }
  return false;
}

// print all vars - for debugging

/*static void dump_vars(void) {
  kprintf("\nfcn_free_bowls: %d,free bowls: %d, cats wait: %d, mice wait: %d, eating cat: %d, eating mice: %d\n", get_free_bowl(), num_free_bowls,cats_waiting,mice_waiting,eating_cats_count,eating_mice_count);
}*/


static
void
cat_simulation(void * unusedpointer, 
               unsigned long catnumber)
{
  int i;
  unsigned int bowl;

  /* avoid unused variable warnings. */
  (void) unusedpointer;
  (void) catnumber;


  /* your simulated cat must iterate NumLoops times,
   *  sleeping (by calling cat_sleep() and eating
   *  (by calling cat_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

	  lock_acquire(bowl_lock);

    cats_waiting++;
    while (get_free_bowl()==0 || if_eating(2)) {
      cv_wait(free_cv,bowl_lock);
    }
    cats_waiting--;
    cv_broadcast(free_cv, bowl_lock);

    bowl = get_free_bowl();
    bowl_status[bowl-1] = 1;
    num_free_bowls--;
    eating_cats_count++;
    lock_release(bowl_lock);
    cat_eat(bowl, CatEatTime);
    bowl_status[bowl-1] = 0;
    num_free_bowls++;
    eating_cats_count--;

    lock_acquire(bowl_lock);
    cv_broadcast(free_cv,bowl_lock);
    lock_release(bowl_lock);

    cat_sleep(CatSleepTime);
    
  }

  /* indicate that this cat simulation is finished */
  V(CatMouseWait); 
}

/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 *
 */

static
void
mouse_simulation(void * unusedpointer,
          unsigned long mousenumber)
{
  int i;
  unsigned int bowl;

  /* Avoid unused variable warnings. */
  (void) unusedpointer;
  (void) mousenumber;

  for(i=0;i<NumLoops;i++) {

	  lock_acquire(bowl_lock);
    mice_waiting++;
    while (get_free_bowl()==0 || if_eating(1) || cats_waiting > 0) {
  	  cv_wait(free_cv,bowl_lock);
    }
  	mice_waiting--;
    
    bowl = get_free_bowl();

    bowl_status[bowl-1] = 2;
    eating_mice_count++;
  	num_free_bowls--;
    lock_release(bowl_lock);
    mouse_eat(bowl, MouseEatTime);
    bowl_status[bowl-1] = 0;
    num_free_bowls++;
    eating_mice_count--;
    
    lock_acquire(bowl_lock);
    cv_broadcast(free_cv,bowl_lock);
    lock_release(bowl_lock);

    mouse_sleep(MouseSleepTime);


  }

  /* indicate that this mouse is finished */
  V(CatMouseWait); 
}


/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5 or 9
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 * Optional parameters
 *                    args[5] = cat eating time
 *                    args[6] = cat sleeping time
 *                    args[7] = mouse eating time
 *                    args[8] = mouse sleeping time
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 *      You may need to modify this function, e.g., to
 *      initialize synchronization primitives used
 *      by the cat and mouse threads.
 *      
 *      However, you should should ensure that this function
 *      continues to create the appropriate numbers of
 *      cat and mouse threads, to initialize the simulation,
 *      and to wait for all cats and mice to finish.
 */

int
catmouse(int nargs,
         char ** args)
{
  int index, error;
  int i;


  /* check and process command line arguments */
  if ((nargs != 9) && (nargs != 5)) {
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
    kprintf("or\n");
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS ");
    kprintf("CAT_EATING_TIME CAT_SLEEPING_TIME MOUSE_EATING_TIME MOUSE_SLEEPING_TIME\n");
    return 1;  // return failure indication
  }

  /* check the problem parameters, and set the global variables */
  NumBowls = atoi(args[1]);
  if (NumBowls <= 0) {
    kprintf("catmouse: invalid number of bowls: %d\n",NumBowls);
    return 1;
  }
  NumCats = atoi(args[2]);
  if (NumCats < 0) {
    kprintf("catmouse: invalid number of cats: %d\n",NumCats);
    return 1;
  }
  NumMice = atoi(args[3]);
  if (NumMice < 0) {
    kprintf("catmouse: invalid number of mice: %d\n",NumMice);
    return 1;
  }
  NumLoops = atoi(args[4]);
  if (NumLoops <= 0) {
    kprintf("catmouse: invalid number of loops: %d\n",NumLoops);
    return 1;
  }

  if (nargs == 9) {
    CatEatTime = atoi(args[5]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid cat eating time: %d\n",CatEatTime);
      return 1;
    }
  
    CatSleepTime = atoi(args[6]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid cat sleeping time: %d\n",CatSleepTime);
      return 1;
    }
  
    MouseEatTime = atoi(args[7]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid mouse eating time: %d\n",MouseEatTime);
      return 1;
    }
  
    MouseSleepTime = atoi(args[8]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid mouse sleeping time: %d\n",MouseSleepTime);
      return 1;
    }
  }


  // Initialize some variables
  init();

  kprintf("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
          NumBowls,NumCats,NumMice,NumLoops);
  kprintf("Using cat eating time %d, cat sleeping time %d\n", CatEatTime, CatSleepTime);
  kprintf("Using mouse eating time %d, mouse sleeping time %d\n", MouseEatTime, MouseSleepTime);

  /* create the semaphore that is used to make the main thread
     wait for all of the cats and mice to finish */
  CatMouseWait = sem_create("CatMouseWait",0);
  if (CatMouseWait == NULL) {
    panic("catmouse: could not create semaphore\n");
  }

  /* 
   * initialize the bowls
   */
  if (initialize_bowls(NumBowls)) {
    panic("catmouse: error initializing bowls.\n");
  }

  /*
   * Start NumCats cat_simulation() threads.
   */
  for (index = 0; index < NumCats; index++) {
    error = thread_fork("cat_simulation thread", NULL, cat_simulation, NULL, index);
    if (error) {
      panic("cat_simulation: thread_fork failed: %s\n", strerror(error));
    }
  }

  /*
   * Start NumMice mouse_simulation() threads.
   */
  for (index = 0; index < NumMice; index++) {
    error = thread_fork("mouse_simulation thread", NULL, mouse_simulation, NULL, index);
    if (error) {
      panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
    }
  }

  /* wait for all of the cats and mice to finish before
     terminating */  
  for(i=0;i<(NumCats+NumMice);i++) {
    P(CatMouseWait);
  }

  /* clean up the semaphore that we created */
  sem_destroy(CatMouseWait);

  /* clean up resources used for tracking bowl use */
  cleanup_bowls();

  return 0;
}

/*
 * End of catmouse.c
 */
