#include "uthread.h"
#include "user.h"

uint nexttid = 1;
int numOfThreads = 0;
struct uthread* current;
struct uthread u_table[MAX_UTHREADS];

struct bisem* bisemtable[MAX_BSEM+1];


// void
// uthread_schedule_wrapper(int signum){
//   uint tf;
//   asm("movl %%ebp, %0;" :"=r"(tf) : :);
//   tf+=8;
//   uthread_schedule((struct trapframe*)tf);
// }

int
uthread_init()
{
  struct uthread* mainT = &u_table[0];
  mainT->tid = nexttid++;
  mainT->pid = getpid();
  mainT->state = T_RUNNING;
  mainT->t_stack = 0; 
  mainT->ind=0;

  numOfThreads++;
  current = mainT;
  signal(SIGALRM, (sighandler_t) uthread_schedule);
  sigsend(mainT->pid, SIGALRM);
  return mainT->tid;
}

int
uthread_create(void (*start_func)(void*), void* arg)
{
  alarm(0);//disabling SIGALARM interupts to make uthread_create an atomic method
  struct uthread *nextT;
  uint newSp;
  int count=0;

  for(nextT = u_table; nextT < &u_table[MAX_UTHREADS]; nextT++){
    if(nextT->state == T_UNUSED){
      goto found;
    }
    count++;
  }
  //arrive here if there is no more room for another threads
  return -1;

  found:
    numOfThreads++;
    nextT->ind=count;
    nextT->tid = nexttid++;
    nextT->pid = getpid();
    nextT->tf = current->tf;

    
    if(!nextT->t_stack){
      if((nextT->t_stack = (uint)malloc(STACKSIZE)) == 0){
        nextT->state = T_UNUSED;
        sigsend(current->pid, SIGALRM);
        return -1;
      }
    }


    newSp= nextT->t_stack + STACKSIZE;
    // push arg
    newSp -= 4;
    *(void**)newSp = arg;
    // push return address to thread_exit
    newSp -= 4;
    *(void**)newSp = uthread_exit;
    // initialize thread stack pointers
    nextT->tf.esp = newSp;
    // set threads eip to start_func
    nextT->tf.eip = (uint)start_func;
    nextT->state = T_RUNNABLE;
    alarm(UTHREAD_QUANTA);//allowing SIGALARM to interupt again
    return nextT->tid;
}

uint findTrapframe(uint sp){
	uint ans=sp;
	uint runner=*(int*)ans;
	while(runner!=0xABCDEF){
		ans+=4;
		runner=*(int*)ans;
	}
	ans-=sizeof(struct trapframe);
	return ans;
}

void
uthread_schedule()
{	
  printf(2,"-------------ticks are:%d\n---------",uptime());
  alarm(0);//disabling alarms to prevent synchronization problems
  uint sp;
  struct uthread *ut = current;
  //get current esp
  asm("movl %%ebp, %0;" :"=r"(sp) : :);

 if(ut->state == T_RUNNING){
    ut->state = T_RUNNABLE;
  }
  
  uint tfadd=findTrapframe(sp);  //finds the trapframe on the stack
  //backup the trapframe at the uthread struct
  memmove((void*)&ut->tf,(void*)tfadd , sizeof(struct trapframe));
 
  ut++;
  if(ut >= &u_table[MAX_UTHREADS])
    ut = u_table;
  
	
  while(ut->state != T_RUNNABLE){
    if(ut->state == T_SLEEPING && ut->wakeup > 0 && ut->wakeup <= uptime()){
      ut->wakeup = 0;
      ut->state = T_RUNNABLE;
      break;
    } else if(ut->state == T_UNUSED && ut->t_stack && ut->tid != current->tid && ut->tid != 1){
      free((void*)ut->t_stack);
      ut->t_stack = 0;
    }
    ut++;
    if(ut >= &u_table[MAX_UTHREADS])
      ut = u_table;
  }

  // copy the tf of the thread to be run next on to currnt user stack so we will rever back to it at sigreturn;
  memmove((void*)tfadd, (void*)&ut->tf, sizeof(struct trapframe));

  current = ut;
  ut->state = T_RUNNING;
  alarm(UTHREAD_QUANTA);
  return;
}

void
uthread_exit()
{
  alarm(0);//disabling alarms to prevent synchronization problems
  numOfThreads-=1;
  if(numOfThreads <= 1){
  	//freeing all the stacks int the u_table;
    for(struct uthread* ut = u_table; ut < &u_table[MAX_UTHREADS]; ut++){
      if(ut->t_stack){
        free((void*)ut->t_stack);
        ut->t_stack = 0;
      }
    }
    exit();
  }

  //wakes up all the the threads waiting to join the thread
  for(struct uthread* ut = u_table; ut < &u_table[MAX_UTHREADS]; ut++){
    if(ut->state == T_SLEEPING && ut->joining == current->tid){
      ut->joining = 0;
      ut->state = T_RUNNABLE;
    }
  }

  current->state = T_UNUSED;
  sigsend(current->pid, SIGALRM);//instead of allowing alarms we send the signal and go to schedule where alarms will be allowed again
  return;
}

int
uthread_self()
{
  return current->tid;
}

int
uthread_join(int tid)
{
  alarm(0);
  for(struct uthread* ut = u_table; ut < &u_table[MAX_UTHREADS]; ut++){
    if(ut->tid == tid && ut->state != T_UNUSED){
      current->joining = tid;
      current->state = T_SLEEPING;
      sigsend(current->pid, SIGALRM);
      return 0;
    }
  }
  alarm(UTHREAD_QUANTA); //allowing alarms again
  return -1;
}

int uthread_sleep(int ticks)
{
  alarm(0);
  uint current_ticks = uptime();
  if(ticks < 0){
    sigsend(current->pid, SIGALRM);//instead of allowing alarms we send the signal and go to schedule where alarms will be allowed again
    return -1;
  }
  else if(ticks == 0){
    current->wakeup = 0;
  }
  else{
    current->wakeup = ticks+current_ticks;
  }
  current->state = T_SLEEPING;
  sigsend(current->pid, SIGALRM);
  return 0;
}


//this function return the next available semaphore descriptor
//return descriptor of 1-MAX_BISEM id sucsess and -1 if fail
int findNextDecriptor(){
	int ans=-1;;
	for(int i=1;i<MAX_BSEM+1;i++){
		if(bisemtable[i]==0){
			ans=i;
			break;
		}
	}
	return ans;
}

 int bsem_alloc(void){
 	
 	int desc=findNextDecriptor();
 	

 	bisemtable[desc]=malloc(sizeof(struct bisem));
 	
 	bisemtable[desc]->s=1;
 	bisemtable[desc]->lastWaitingInd=0;
 	bisemtable[desc]->totalWaitingThreads=0;


 	return desc;
 }
 void bsem_free(int sem){
 	if(bisemtable[sem]->totalWaitingThreads==0){
 		free(bisemtable[sem]);
 		bisemtable[sem]=0;
 	}
 	else{
 		printf(2,"There are still threads waiting on semaphore");
 	}
 	
 }
 void bsem_down(int sem){
 	alarm(0);
 	struct bisem* thisSem=bisemtable[sem];
 	if(thisSem->s==0){
 		thisSem->s=1;
 	}
 	else{
 		//insert the waiting thread index at u_table  to to the semaphre queue
 		thisSem->waitingThreads[thisSem->lastWaitingInd]=current->ind;
 		uthread_sleep(0);
 		thisSem->lastWaitingInd++;
 		thisSem->totalWaitingThreads++;
 	}
 	alarm(UTHREAD_QUANTA);
 }
 void bsem_up(int sem){
 	struct bisem* thisSem=bisemtable[sem];

 	alarm(0);
 	if(thisSem->totalWaitingThreads>0){
 		int firstInQ=thisSem->waitingThreads[0];
 		u_table[firstInQ].state=T_RUNNABLE;
 		int i=0,j=1;
 		while(j<thisSem->lastWaitingInd){
 			thisSem->waitingThreads[i]=thisSem->waitingThreads[j];
 		}
 		
 		thisSem->waitingThreads[thisSem->lastWaitingInd]=0;
 		thisSem->lastWaitingInd--;
 		thisSem->totalWaitingThreads--;
 		
 		if(thisSem->totalWaitingThreads==0){
 			thisSem->s=1;
 		}
 	}
 	alarm(UTHREAD_QUANTA);
 }
 struct counting_semaphore* csem_alloc(int init_val){
 	struct counting_semaphore* newCSem=malloc(sizeof(struct counting_semaphore));

 	newCSem->s1=bsem_alloc();
 	newCSem->s2=bsem_alloc();
 	if(init_val<1){
 		bsem_down(newCSem->s2);
 	}
 	newCSem->val=init_val;
 	return newCSem;

 }
  void csem_free(struct counting_semaphore* cSem){
  	bsem_free(cSem->s1);
  	bsem_free(cSem->s2);
  	free(cSem);

  }
 void down(struct counting_semaphore* cSem){
 bsem_down(cSem->s2);
  bsem_down(cSem->s1);
  cSem->val--;
  if(cSem->val > 0)
    bsem_up(cSem->s2);
  bsem_up(cSem->s1);
 }
 void up(struct counting_semaphore* cSem){
  bsem_down(cSem->s1);
  cSem->val++;
  if(cSem->val ==1)
    bsem_up(cSem->s2);
  bsem_up(cSem->s1);
 }

