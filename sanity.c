#include "types.h"
#include "user.h"
#include "stat.h"
#include "uthread.h"




#define arrSize 100                                                    
#define num_consumers 3                             
#define num_producers 1

void producer(void* arg);
void consumer(void* arg);
struct counting_semaphore *mutex;                   
struct counting_semaphore *empty;                   
struct counting_semaphore *full;                    
int consumers_bsem_desc;                          
int queue[arrSize];                                  
int queue_next = 0;                      
int consumerTable[num_consumers];
int producerTable[num_producers];
int consumed_items_number = 0;




