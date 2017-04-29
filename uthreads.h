enum threadState { T_UNUSED,T_EMBRYO,T_SLEEPING, T_RUNNABLE, T_RUNNING };
#define MAX_UTHREADS 64
#define UTHREAD_QUANTA 5
#define STACK_SIZE  4096

struct thread {
  int tid;
  int state;
  int lock;
  //---for debugging
  int name;
  uint *t_stack;
  uint t_esp;
  uint t_eip;
};
 
