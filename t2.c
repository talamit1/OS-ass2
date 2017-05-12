/**
wait for all threads to die. then exit()
**/

#include "types.h"
#include "user.h"
#include "stat.h"
#include "uthread.h"

#define NUM_OF_ITERATIONS 75

void tthread1(void *arg) {
  int i;
  for(i = 0; i < NUM_OF_ITERATIONS; i++) {
    printf(1, " 11111111111111111111111111111111111111111111111111111 %d \n",i);
  }
}

void tthread2(void *arg) {
  int i;
  for(i = 0; i < NUM_OF_ITERATIONS; i++) {
    printf(1, " 2222222222222222222222222222222222222222222222222222 %d \n",i);
  }
}

void tthread3(void *arg) {
  int i;
  for(i = 0; i < NUM_OF_ITERATIONS; i++) {
    printf(1, " 3333333333333333333333333333333333333333333333333333 %d \n",i);
  }
}

int main(int argc, char *argv[]) {
	int id1,id2,id3;
	printf(1,"-=-=-=-=-=-=-=-=-=-=-TEXT USER LEVEL THREAD-=-=-=-=-=-=-=-=-=-=-\n");
	uthread_init();
	id1= uthread_create(tthread1, (void*)555);
	id2= uthread_create(tthread2, (void*)555);
	id3= uthread_create(tthread3, (void*)555);
	
	uthread_join(id1);
	//printf(1, "\n **************id2 is %d \n", id2);
	uthread_join(id2);
	uthread_join(id3);
	
	exit();
}