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
  uint joining;
  int wakeup; //the time for the thread to wake-up if sleeping
};




//uthread.c
int uthread_init(void);
int uthread_create(void (*) (void*), void*);
void uthread_schedule();
void uthread_exit(void);
int uthread_self(void);
int uthread_join(int);
int uthread_sleep(int);



