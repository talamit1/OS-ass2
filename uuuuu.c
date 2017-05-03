#include "types.h"
#include "user.h"
#include "uthreads.h"
#include "x86.h"

static struct uthread threads[MAX_UTHREADS];
int next_tid;
int currThreadInd;
int numOfThreads;



int uthread_init(){
  int i;
  for(i=0;i<MAX_UTHREADS;i++){
    threads[i].state=T_UNUSED;
  }
  //threads[0].state=T_RUNNING;
  next_tid=1;

  currThreadInd=0;
  numOfThreads=0;
  threads[currThreadInd].tid = 0;
  threads[currThreadInd].t_stack = 0; 
  threads[currThreadInd].state = T_RUNNING;
  threads[currThreadInd].t_esp=1;
  threads[currThreadInd].t_ebp=1;
  
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

  *((int*)(threads[i].t_stack + STACK_SIZE - 3*addrSize)) = 0; // This return address will never be used (The run_thread_func will never return)
  *((int*)(threads[i].t_stack + STACK_SIZE - 2*addrSize)) = (int)start_func;
  *((int*)(threads[i].t_stack + STACK_SIZE - 1*addrSize)) = (int)arg;
  threads[i].t_ebp=(int)threads[i].t_stack+STACK_SIZE-3*addrSize;

  //set wakwMe to -1; check in schduler each time if wakeMeAt ==0
  threads[i].wakeMeAt=-1;
  printf(2,"thread num: %d created\n",i );
  threads[i].t_esp=0;
  threads[i].t_ebp=0;
  threads[i].state=T_RUNNABLE;
  numOfThreads++;

  alarm(UTHREAD_QUANTA);
  return threads[i].tid;
} 

static void run_thread_func(void (*start_func)(void *), void* arg) {
  alarm(UTHREAD_QUANTA);
  start_func(arg);

  uthread_exit();
  }

void uthread_schedule(){
  printf(2,"------------------------ticks are: %d ------------------\n",uptime());

  // This is necessary incase uthread_yield was explicitly called rather than
  // being called from the alarm signal:
  alarm(0);


  if (threads[currThreadInd].state == T_RUNNING) {
    threads[currThreadInd].state = T_RUNNABLE;
  }

  //backup reg



  //threads[currThreadInd]. ut_tf =  (struct trapframe)(threads[currThreadInd].t_esp)+8;

  // Switch to new thread
  asm("movl %%esp, %0;" : "=r" (threads[currThreadInd].t_esp));
  asm("movl %%ebp, %0;" : "=r" (threads[currThreadInd].t_ebp));
  currThreadInd++;
  while(threads[currThreadInd].state != T_RUNNABLE) {
    currThreadInd++;
    if(currThreadInd == MAX_UTHREADS) {
      currThreadInd = 0;  
    }
  }
   if(threads[currThreadInd].tid==0){
    currThreadInd++;

  }


  threads[currThreadInd].state = T_RUNNING;
  printf(2,"cirretn thread is:%d\n",currThreadInd);
  if(threads[currThreadInd].t_esp == 0) { 
 printf(2,"esp is  \n" );
    asm("movl %0, %%esp;" : : "r" (threads[currThreadInd].t_stack + STACK_SIZE - 3*sizeof(int)));
    asm("movl %0, %%ebp;" : : "r" (threads[currThreadInd].t_stack + STACK_SIZE - 3*sizeof(int)));
   
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

    printf(2,"ssssssssssssssssss\n" );
    alarm(0);
      
    if(threads[currThreadInd].t_stack)
      free(threads[currThreadInd].t_stack);

    threads[currThreadInd].state=T_UNUSED;
    threads[currThreadInd].t_stack=0;
         



    //check if there is a live thread
    int i;
    int numOfRunningThreads=1;
    for(i=1;i<MAX_UTHREADS;i++){
      if(threads[i].state!=T_UNUSED){
          numOfRunningThreads++;
        }
      }
    

    if(numOfRunningThreads==1)
      exit();

    sigsend(14,getpid());
}

int uthred_self(){
  return threads[currThreadInd].tid;

}

///////////////////////////////////////////////////////////////////

int  uthred_join(int tid)
{
  
  if(tid < 0 || tid >= next_tid){
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
      sigsend(14,getpid());

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
  
  if(ticks < 0) 
    return -1;  
  
  //critical code
  threads[currThreadInd].wakeMeAt = uptime()+ticks;
  threads[currThreadInd].state=T_SLEEPING;

  alarm(0);
  sigsend(14,getpid());
    
  return 0;
}

////task 3 semaphores////////////////
int bsem_alloc(){

  struct binarySemaphore* mysem = malloc(sizeof (struct binarySemaphore));
  mysem->count= 1;
  mysem->state=UNLOCK;
  mysem->numOfWaitings=0;
  mysem->firstThreadInLine=0;
  mysem->lasThreadInLine=0;
  return (int)mysem;
}
void freeThreadQueue(struct binarySemaphore *sem){
struct Link* temp=sem->firstThreadInLine;
    while(temp->next!=0){
      struct Link* t=temp;
      temp=temp->next;
      free(t);
    }
    free(temp);
  }
void bsem_free(int semDec){
  struct binarySemaphore *sem =(struct binarySemaphore*) semDec;
  if(sem->numOfWaitings>0){
    freeThreadQueue(sem);
   }
  free(sem);
}

void bsem_down(int semDec){
  struct binarySemaphore *sem =(struct binarySemaphore*) semDec;
  if(sem->state==UNLOCK && sem->numOfWaitings==0)
    sem->state=LOCK;
  else{
    //this in case the locked is already aquired
    struct Link* newWait=malloc(sizeof (struct Link));
    int myTid=uthred_self();
    newWait->data=myTid;
    newWait->next=0;
    if(sem->numOfWaitings==0){
      //the case it is the first thread to wait
      sem->firstThreadInLine=newWait;
      
    }
    else{
      //insert the new thread at the end of the queue
      sem->lasThreadInLine->next=newWait;
      sem->lasThreadInLine=sem->lasThreadInLine->next;
  }
  sem->numOfWaitings++;
  threads[myTid].state=T_SLEEPING;
  sigsend(14,getpid());
  }

 }
void bsem_up(int semDec){
 struct binarySemaphore *sem =(struct binarySemaphore*) semDec;
 if(sem->count==1)
  return;
 else{
    // Wake up all waiting  threads
    struct Link* l=sem->firstThreadInLine;
    while(l!=0){
      threads[l->data].state=T_RUNNABLE;

    }
    sem->state=UNLOCK;
    sem->count=1;
    sem->numOfWaitings=0;
    sigsend(14,getpid());


 }



}
