#include "types.h"
#include "user.h"
#include "uthreads.h"
#include "x86.h"

static struct uthread threads[MAX_UTHREADS];
int next_tid;
int currThreadInd;
int nunOfThreads;


int uthread_init(){
	int i;
	for(i=0;i<MAX_UTHREADS;i++){
		threads[i].state=T_UNUSED;
	}
	//threads[0].state=T_RUNNING;
	next_tid=1;

	currThreadInd=0;
	threads[currThreadInd].tid = 0;
  threads[currThreadInd].t_stack = 0; 
  threads[currThreadInd].state = T_RUNNING;
  
  if(signal(14,uthread_schedule)!=0)
  	return -1;
  
  alarm(UTHREAD_QUANTA);
  return 0;

}
	
int uthread_create(void (*start_func)(void *), void*arg){
  

  alarm(0);

  int i;
  //int found=0;
  for(i = 0;i<MAX_UTHREADS /*&& !found*/; i++)
    if(threads[i].state == T_UNUSED)
      goto found;

  alarm(UTHREAD_QUANTA);  //case we didn't have any free threads
  return -1;
  
  found:
  threads[i].tid=next_tid;
  next_tid++;
  int addrSize=sizeof(int);
  threads[i].t_stack=malloc(STACK_SIZE);
  *((int*)(threads[i].t_stack + STACK_SIZE - 3*addrSize)) = 0;
  *((int*)(threads[i].t_stack + STACK_SIZE - 2*addrSize)) = (int)start_func;
  *((int*)(threads[i].t_stack + STACK_SIZE - 1*addrSize)) = (int)arg;

  threads[i].t_esp=0;
  threads[i].t_ebp=0;
  threads[i].state=T_RUNNABLE;

  alarm(UTHREAD_QUANTA);
  return threads[i].tid;
} 

static void run_thread_func(void (*start_func)(void *), void* arg) {
  alarm(UTHREAD_QUANTA);
  start_func(arg);
  uthread_exit();
}

void uthread_schedule(){
  alarm(0);

  //change self thread to runnable mode
  if(threads[currThreadInd].state==T_RUNNING)
    threads[currThreadInd].state=T_RUNNABLE;
  
  asm("movl %%esp, %0;" : "=r" (threads[currThreadInd].t_esp));
  asm("movl %%ebp, %0;" : "=r" (threads[currThreadInd].t_ebp));



  //update currThreadInd to next in turn
  currThreadInd++;
  while(threads[currThreadInd].state != T_RUNNABLE) {
    currThreadInd++;
    if(currThreadInd == MAX_UTHREADS) {
      currThreadInd = 0;
    }
  }

  //set the new thread to running
  threads[currThreadInd].state=T_RUNNING;


////////////////////////////////////////////////////////////////////////////////
   if(threads[currThreadInd].t_esp == 0) {
    // First time the thread is being run. Set the stack to initial values and
    // jump to the run_thread_func

    int addrSize=sizeof(int);

    asm("movl %0, %%esp;" : : "r" (threads[currThreadInd].t_stack + STACK_SIZE - 3*addrSize));
    asm("movl %0, %%ebp;" : : "r" (threads[currThreadInd].t_stack + STACK_SIZE - 3*addrSize));

    asm("jmp *%0;" : : "r" (run_thread_func));
  } else {
    // The thread is already running. Restore its stack, and then when the
    // current call to uthread_yield returns, execution will return to where
    // the thread was previously executing.

    asm("movl %0, %%esp;" : : "r" (threads[currThreadInd].t_esp));
    asm("movl %0, %%ebp;" : : "r" (threads[currThreadInd].t_ebp));

    alarm(UTHREAD_QUANTA);
  }
}
///////////////////////////////////////////////////////////////////////////////
void uthread_exit(){

      
    alarm(0);
    
    if(threads[currThreadInd].t_stack)
      free(threads[currThreadInd].t_stack);

    threads[currThreadInd].state=T_UNUSED;


    //join
    int j;
    for(j = 0; j < MAX_UTHREADS; ++j) {
      if (threads[j].state == T_SLEEPING) 
        threads[j].state = T_RUNNABLE;
    }


    //check if there is a live thread
    int i;
    for(i=0;i<MAX_UTHREADS;i++){
      if(threads[i].state!=T_UNUSED)
        uthread_schedule();
    }

    exit();
}

int uthred_self(){
	return threads[currThreadInd].tid;

}


///////////////////////////////////////////////////////////////////

int  uthred_join(int tid)
{
  

  if(tid < 0 || tid >= next_tid) {
    return -1;
  }

search:

  // The following is a critical section, so disable thread switching
  alarm(0);

  int i;
  for(i = 0; i < MAX_UTHREADS; ++i) {
    if(threads[i].state != T_UNUSED && threads[i].tid == tid) {
      // Found the matching thread. Put the current thread to sleep
      threads[currThreadInd].state = T_SLEEPING;
      uthread_schedule();

      // We arrive here after the current thread has been woken up. Jump up and
      // try again
      goto search;
    }
  }

  // If we got to here, then the requested tid is not running and must have
  // already completed.

  // Re-enable thread switching
  alarm(UTHREAD_QUANTA);
  return 0;
}


int uthred_sleep(int ticks){
	return 0;
}




