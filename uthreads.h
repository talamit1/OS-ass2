enum threadState { T_UNUSED,T_SLEEPING, T_RUNNABLE, T_RUNNING };
#define MAX_UTHREADS 64
#define UTHREAD_QUANTA 5
#define STACK_SIZE  4096

struct uthread {
  int tid;
  enum threadState state;
  //int lock;

  char *t_stack;
  int t_esp;
  int t_ebp;
  
  //---for debugging
  int name;
 
};

int uthread_init();
int uthread_create(void (*start_func)(void *), void*arg);
void uthread_schedule();
void uthread_exit();
int uthred_self();
int uthred_join(int tid);
int uthred_sleep(int ticks);