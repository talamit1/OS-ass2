enum threadState { T_UNUSED,T_SLEEPING, T_RUNNABLE, T_RUNNING };
enum semState{ LOCK,UNLOCK };
#define MAX_UTHREADS 64
#define UTHREAD_QUANTA 5
#define STACK_SIZE  4096

#define MAX_BSEM 128.


struct uthread {
  int tid;
  enum threadState state;
  int wakeMeAt;
  //int lock;

  char *t_stack;
  int t_esp;
  int t_ebp;
  

 
};
struct Link{
	int data;
	struct Link *next;
};


struct binarySemaphore{
	struct Link *firstThreadInLine;
	struct Link *lasThreadInLine;
	int runingTid;
	int count;
	enum semState state;
	int numOfWaitings;
	
};
struct countingSemaphore{
	
	int count;
	enum semState state;
};

int uthread_init();
int uthread_create(void (*start_func)(void *), void*arg);
void uthread_schedule();
void uthread_exit();
int uthred_self();
int uthred_join(int tid);
int uthred_sleep(int ticks);
int bsem_alloc();
void bsem_free(int); 
void bsem_down(int);
void bsem_up(int);