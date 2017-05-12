<<<<<<< HEAD
#include "types.h"
#include "x86.h"
#include "param.h"


enum tstate { T_UNUSED, T_SLEEPING, T_RUNNABLE, T_RUNNING};

struct uthread {
  uint tid;   //thread id
  uint t_stack; //pointer to the thread's stack
  enum tstate state; //thread state (UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE)
  struct trapframe tf; //the trapframe backed up on the user stack
  uint pid; //the pid of the process running this thread
  uint numOfJoining;  //number of threads who waits on this thread
  uint joiningList[MAX_UTHREADS];        //the threads who waits on this thread by indexes
  int wakeup; //the time for the thread to wake-up if sleeping
  int t_ind;
};

struct bound_queue{
  int in;
  int out;
  int contains;
  int size;
  void** queue;
};


struct bsem{
  uint s;
  int insert;
  int toOut;
  int size;
  int numOfWatingThreads;
  void** list;
};

struct counting_semaphore{
  int val;
  int s1;
  int s2;
};

//uthread.c
int uthread_init(void);
int uthread_create(void (*) (void*), void*);
void uthread_schedule(void);
void uthread_exit(void);
int uthread_self(void);
int uthread_join(int);
int uthread_sleep(int);




int bsem_alloc(void);
void bsem_free(int);
void bsem_down(int);
void bsem_up(int);
struct counting_semaphore* csem_alloc(int init_val);
void csem_free(struct counting_semaphore*);
void down(struct counting_semaphore*);
void up(struct counting_semaphore*);
=======
#include "types.h"
#include "x86.h"
#include "param.h"


enum tstate { T_UNUSED, T_SLEEPING, T_RUNNABLE, T_RUNNING};//, ZOMBIE, JOINNING };

struct uthread {
  uint tid;   //thread id
  uint t_stack; //pointer to the thread's stack
  enum tstate state; //thread state (UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE)
  struct trapframe tf; //the trapframe backed up on the user stack
  uint pid; //the pid of the process running this thread
  int ind;
  uint joining;
  int wakeup; //the time for the thread to wake-up if sleeping
};


struct bisem{
	int s;
	int waitingThreads[MAX_UTHREADS];
	int lastWaitingInd;
	int totalWaitingThreads;
};

struct counting_semaphore{
	int val;
	int s1;
	int s2;
};

//task2
//uthread.c
int uthread_init(void);
int uthread_create(void (*) (void*), void*);
void uthread_schedule();
void uthread_exit(void);
int uthread_self(void);
int uthread_join(int);
int uthread_sleep(int);


//task3
int bsem_alloc(void);
void bsem_free(int);
void bsem_down(int);
void bsem_up(int);


struct counting_semaphore* csem_alloc(int init_val);
void csem_free(struct counting_semaphore*);
void down(struct counting_semaphore*);
void up(struct counting_semaphore*);



>>>>>>> d445f1485d79782627e7ffb832b538acdeaca7b6
