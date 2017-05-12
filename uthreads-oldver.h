enum threadState { T_UNUSED,T_SLEEPING, T_RUNNABLE, T_RUNNING };
enum semState{ LOCK,UNLOCK };
#define MAX_UTHREADS 64
#define UTHREAD_QUANTA 5
#define STACK_SIZE  4096

#define MAX_BSEM 128.


struct trapframe {
  // registers as pushed by pusha
  uint edi;
  uint esi;
  uint ebp;
  uint oesp;      // useless & ignored
  uint ebx;
  uint edx;
  uint ecx;
  uint eax;

  // rest of trap frame
  ushort gs;
  ushort padding1;
  ushort fs;
  ushort padding2;
  ushort es;
  ushort padding3;
  ushort ds;
  ushort padding4;
  uint trapno;

  // below here defined by x86 hardware
  uint err;
  uint eip;
  ushort cs;
  ushort padding5;
  uint eflags;

  // below here only when crossing rings, such as from user to kernel
  uint esp;
  ushort ss;
  ushort padding6;
};


struct uthread {
  int tid;
  enum threadState state;
  int wakeMeAt;
  int isFirstRun;
  int t_eip;
  char *t_stack;
  int t_esp;
  int t_ebp;
  struct trapframe t_tf;
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