#include "uthread.h"
#include "user.h"

#define N 100 // buffer size;

int mutex;
struct counting_semaphore* empty;
struct counting_semaphore* full;
int in=0;
int out=0;
int arr[100];

void
producer(void* arg)
{
  for(int i = 1; i<1004; i++){
      down(empty);
      bsem_down(mutex);
      arr[in]=i;
      in=(in+1)%N;
      bsem_up(mutex);
      up(full);
  }
}

void
consumer(void* arg) {
  int item;
  while(1){
    down(full);
    bsem_down(mutex);
    item = arr[out];
    arr[out]=-1;
    out=(out+1)%N;
    if(item > 1000){
      bsem_up(mutex);
      up(empty);
      break;
    }
    bsem_up(mutex);
    up(empty);
    //printf(1,"Thread %d going to sleep for %d ticks.\n",uthread_self(),item);
    uthread_sleep(item);
    printf(1,"Thread %d slept for %d ticks.\n",uthread_self(),item);
  }
}


int
main(int argc, char *argv[]){

  int consumers[3];


  uthread_init();
  mutex = bsem_alloc();
  empty = csem_alloc(N);
  full = csem_alloc(0);


  for(int i = 0; i<3; i++){
    consumers[i] = uthread_create(consumer,0);
  }

  int prod = uthread_create(producer,0);
  uthread_join(prod);

  for(int i = 0; i<3; i++){
    uthread_join(consumers[i]);
  }
  

  bsem_free(mutex);
  
  csem_free(empty);
  
  csem_free(full);
  

  printf(1,"end of Sanity\n");
  uthread_exit();
}