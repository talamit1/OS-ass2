
#include "types.h"
#include "user.h"
#include "stat.h"
#include "uthreads.h"


typedef void (*sighandler_t)(int);

// void
// printA(void * a){
//     uthred_sleep(20);
//  for(int i=0;i<100;i++){
//     printf(1,"*************************************tid MAY SHRMUTA:%d\n",uthred_self());
//      printf(2,"%d\n" ,i);
// }
// }

void
printB(void * a){
 //uthred_join(1);   
 for(int i=0;i<500;i++){
     printf(1,"*************************************tid:%d\n",uthred_self());
     printf(2,"%d\n" ,i);
}
}

// void
// printC(void * a){
//  //uthred_join(2);   
//  for(int i=200;i<300;i++){
//      printf(1,"*************************************tid:%d\n",uthread_self());
//      printf(2,"%d\n" ,i);
// }
// }

int count=0;
int
main(int argc, char *argv[]){
	printf(2,"main count is: %d\n",count );
	count++;
printf(1,"------------------TestEx2----------------- \n");

uthread_init();
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
uthread_create(printB,0);
printf(1,"*********************1************tid:%d\n",uthred_self());
// for(int i=1;i<20;i++)
// {
//      uthred_sleep(8);
     
// }

printf(1,"ByeByeMain");
exit();
}