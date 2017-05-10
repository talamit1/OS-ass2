#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


void initSigHandlers(struct proc * p) {
    int i;
    for (i = 0; i < NUMSIG; i++) {
            p->handlers[i] = 0;     
    }
}


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  p->pending=0;
  p->alarmFlag =-1;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;
  
  initSigHandlers(p);
  p->pending=0;
  p->isHandelingSignal=0;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
/* ---------------task 1B ------------- */
//register a new signal handler to the process handlers

//function to print trapframe;
void printTrapframe(struct trapframe* tf){
  cprintf("-----------printing trapfram from kernel------\n");
  cprintf("tf->edi: %d\n",tf->edi);
  cprintf("tf->esi: %d\n",tf->esi);
  cprintf("tf->ebp: %d\n",tf->ebp);
  cprintf("tf->oesp: %d\n",tf->oesp);
  cprintf("tf->ebx: %d\n",tf->ebx);
  cprintf("tf->edx: %d\n",tf->edx);
  cprintf("tf->ecx: %d\n",tf->ecx);
  cprintf("tf->eax: %d\n",tf->eax);
  
  cprintf("tf->gs: %d\n",tf->gs);
  cprintf("tf->padding1: %d\n",tf->padding1);
  cprintf("tf->fs: %d\n",tf->fs);
  cprintf("tf->padding2: %d\n",tf->padding2);
  cprintf("tf->es: %d\n",tf->es);
  cprintf("tf->padding3: %d\n",tf->padding3);
  cprintf("tf->ds: %d\n",tf->ds);
  cprintf("tf->padding4: %d\n",tf->padding4);
  cprintf("tf->trapno: %d\n",tf->trapno);


  cprintf("tf->err: %d\n",tf->err); 
  cprintf("tf->eip: %d\n",tf->eip);
  cprintf("tf->cs: %d\n",tf->cs);
  cprintf("tf->padding5: %d\n",tf->padding5); 
  cprintf("tf->eflags: %d\n",tf->eflags);

  cprintf("tf->esp: %d\n",tf->esp);
  cprintf("tf->ss: %d\n",tf->ss);
  cprintf("tf->padding6: %d\n",tf->padding6);

}

sighandler_t signal(int signum,sighandler_t handler){
  if(!proc || signum<0 || signum>=NUMSIG) 
    return (sighandler_t)-1;
  //acquire(&ptable.lock);
  sighandler_t oldHandler=proc->handlers[signum];
  proc->handlers[signum]=handler;
  //release(&ptable.lock);
  
  return oldHandler;
}
/* ---------------task 1C ------------- */
//this function responsible for the sgnal sending
int sigsend(int pid,int signum){
  struct proc *p;
  acquire(&ptable.lock);
  int found=0;
  for(p=ptable.proc;p<&ptable.proc[NPROC]&& !found ;p++){
    if(p->pid==pid){
      found=1;
      int num=1<<signum;
      proc->pending|=num;
     // cprintf("pending in sigsend is: %d \n",p->pending);
      break;

    }
  }

  release(&ptable.lock);
  if(found==0)
    return -1;

  return 0;

}

void defHandler(int signum){

  cprintf("A signal %d was accepted by process %d\n",signum,proc->pid);

}

int sigreturn(void){ 

  cprintf("-----------proc tf when starting sigreturn:-----\n "); 
  //printTrapframe(proc->tf); 


  int has_lk = holding(&ptable.lock);
  if (!has_lk) acquire(&ptable.lock);
  memmove(proc->tf,(void*)(proc->tf->ebp + 8),sizeof(struct trapframe)); // backup trapframe on user stack
  if (! has_lk) release(&ptable.lock);
  return 0;
  

  
  // cprintf("-----------proc tf at the end of sigreturn:-----\n ");
  // printTrapframe(proc->tf);   
  
  
 
 return 0;
}

void 
sigretwrapper() 
{  
	__asm__ (
          "movl $24, %eax\n" // sigreturn number
          "int     $64");
}



/*------------- task 1D ---------------*/
//this function checks pending signals and handle them if necessery
void checkPendingSignals(){
  if(proc && (proc->tf->cs&3) == DPL_USER   && (proc->pending != 0) ){
    //cprintf("------------------------------------------------------\n");
          
          uint runner=proc->pending;
          uint sigNum;
          if(proc->pending==1)
            sigNum=0;
          else{
          for(sigNum=-1;runner!=0;runner>>=1)
            sigNum++;
        }

          proc->pending-=1<<sigNum;
          
          //cprintf("proc pending is: %d\n",proc->pending);

          sighandler_t handler = proc->handlers[sigNum];         

          if(handler==0){
           
            defHandler(sigNum);
            return;}
 		            
         uint nesp = proc->tf->esp-(&checkPendingSignals-&sigretwrapper);
         // cprintf("-----------proc tf at checking signal it on the stack:-----\n ") ;
         // printTrapframe(proc->tf);
         int retAddress=nesp;
         memmove((void*)nesp,sigretwrapper,&checkPendingSignals-&sigretwrapper);
         nesp-=4;
         uint tfFlag=0xABCDEF;
         memmove((void*)nesp,&tfFlag,sizeof(uint));
         
         nesp-=sizeof(struct trapframe);
         memmove((void*)nesp,proc->tf,sizeof(struct trapframe));

        //cprintf("trapframe put at : %d\n",nesp);
         //cprintf("--------the tf proc->tf int checkingSignals is:--------");
         //printTrapframe(proc->tf);
         //cprintf("--------------------gg------------------");
         //cprintf("--------the tf pushed on the stack int checkingSignals is:--------");
         //printTrapframe((struct trapframe*)&nesp);
         //cprintf("--------------------llll------------------");

         //pushing sigNumber(sighandler arguments)
         nesp=nesp-sizeof(int);
         memmove((void*)nesp,&sigNum,sizeof(int));
         //pushing the sigreturn return adress
         nesp=nesp-sizeof(int*);
         memmove((void*)nesp,&retAddress,sizeof(int));
         cprintf("retAdress put on place: %d\n",nesp);


           int has_lk = holding(&ptable.lock);
           if (!has_lk) acquire(&ptable.lock);
          // change user eip so that user will run the signal handler next
          proc->tf->eip = (uint)proc->handlers[sigNum];
          proc->tf->ebp = nesp;
          proc->tf->esp = nesp;
          if (! has_lk) release(&ptable.lock);
	   }
  }

void updateAlarams(){
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){        
    if(p->alarmFlag >0){
      p->alarmFlag--;
      if(p->alarmFlag==0){
        p->pending|=1<<14;
      }
    }

  }

}
int alarm(int ti){
 
if(ti <0)
  return -1;

int temp = ~1<<14;

if(ti==0){
  proc->pending &= temp;
  return 0;}


  proc->alarmFlag= ti;
return 0;

}





 /* 
  if(ti==0){
    proc->pending ^=1<<14;
  }

  proc-> alarmFlag= ti;

  return 0;
}*/

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
