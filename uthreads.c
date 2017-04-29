#include uthreads.h


static struct uthread threads[MAX_UTHREADS];
int next_tid;
int currThreadInd;


int uthread_init(){
	int i;
	for(i=0i<MAX_UTHREADS;i++){
		threads[i].state=UNUSED;
	}
	thrads[0].state=RUNNING;
	next_tid=1;

	currThreadInd=0;
	threads[current_thread_index].tid = 0;
  threads[current_thread_index].t_stack = 0; 
  threads[current_thread_index].t_state = T_RUNNING;
  if(signal(SIGALARM,uthread_schedule)!=0)
  	return -1;
  alaram(UTHREAD_QUANTA);

}
	
int uthread_create(void (*start_func)(void *), void*arg){
  
  int i;
  int found=0;
  for(i = 0;i<MAX_UTHREADS && !found; i++)
    if(threads[i]->state == UNUSED)
      goto found;

  alarm(UTHREAD_QUANTA);  //case we didn't have any free threads
  return -1;
  
  found:
  threads[i].tid=next_tid;
  next_tid++;
  int addrSize=sizeof(int);
  threads[i].t_stack=malloc(STACK_SIZE);
  *(*uint)thread[i].t_stack+STACK_SIZE-3*addrSize=&uthread_exit; //ibsert return adress to thread exit
  *(uint*)threads[i].t_stack+STACK_SIZE-2*addrSize=arg;
  *(uint*)threads[i].t_stack+STACK_SIZE-1*addrSize=arg;

  threads[i].t_esp=0;
  thread[i].t_ebp=0;
  threads[i].state=T_RUNNABLE;

  alarm(UTHREAD_QUANTA);
  return thread[i].tid;

} 

void uthread_schedule(){

}
/*void uthread_exit(){

	alarm(0);
	if(currThreadInd==0)
		exit();          //maybe need to free all other trheads mallocs

	elas{
		free(threads[currThreadInd].t_stack)	
	}
}*/

int uthred_self(){
	return threads[current_thread_index].tid;

}
int uthred_join(int tid){

}
int uthred_sleep(int ticks){
	-
}




