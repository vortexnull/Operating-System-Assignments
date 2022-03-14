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

#ifdef FCFS

// for FCFS scheduling
// Priority queue of RUNNABLE processes
// based on process creation time
struct {
  int size;
  struct proc *proc[NPROC];
} pqueue;

static int parent(int);
static int leftchild(int);
static int rightchild(int);
static void shiftup(int);
static void shiftdown(int);
static void insert(struct proc*);
static struct proc* remove(void);

static int
parent(int i)
{
  return ((i - 1) / 2);
}

static int
leftchild(int i)
{
  return (2 * i + 1);
}

static int
rightchild(int i)
{
  return (2 * i + 2);
}

static void
shiftup(int i)
{ 
  struct proc *tmp;
  
  while(i > 0 && pqueue.proc[parent(i)]->timecreated > pqueue.proc[i]->timecreated){
    tmp = pqueue.proc[parent(i)];
    pqueue.proc[parent(i)] = pqueue.proc[i];
    pqueue.proc[i] = tmp;

    i = parent(i);
  }
}

static void
shiftdown(int i)
{
  int minindex = i;
  struct proc *tmp;

  if(leftchild(i) < pqueue.size && pqueue.proc[leftchild(i)]->timecreated < pqueue.proc[minindex]->timecreated)
    minindex = leftchild(i);

  if(rightchild(i) < pqueue.size && pqueue.proc[rightchild(i)]->timecreated < pqueue.proc[minindex]->timecreated)
    minindex = rightchild(i);

  if(minindex == leftchild(i)){
    tmp = pqueue.proc[leftchild(i)];
    pqueue.proc[leftchild(i)] = pqueue.proc[i];
    pqueue.proc[i] = tmp;
    shiftdown(minindex);
  }
  else if(minindex == rightchild(i)){
    tmp = pqueue.proc[rightchild(i)];
    pqueue.proc[rightchild(i)] = pqueue.proc[i];
    pqueue.proc[i] = tmp;
    shiftdown(minindex);
  }
}

static void
insert(struct proc *p)
{ 
  if(pqueue.size < NPROC){
    pqueue.proc[pqueue.size] = p;
    shiftup(pqueue.size);
    pqueue.size++;
  }
}

static struct proc*
remove(void)
{
  if(pqueue.size == 0)
    return 0; 

  struct proc *p = pqueue.proc[0];
  pqueue.size--;
  pqueue.proc[0] = pqueue.proc[pqueue.size];
  shiftdown(0);

  return p;
}

#endif

#if defined(MLQ) || defined(DMLQ)

// for MLQ/DMLQ scheduling
// Multilevel queue of RUNNABLE processes
struct {
  int start[NUMPR], end[NUMPR];
  struct proc *proc[NUMPR][NPROC + 1];
} multiqueue;

static int isempty(int);
static void enque(int, struct proc*);
static struct proc* deque(int);
// static void print(void);

static int
isempty(int priority) 
{
  int qindex = priority - 1;

  if(multiqueue.start[qindex] == multiqueue.end[qindex])
    return 1;
  
  return 0;
}

static void
enque(int priority, struct proc *p)
{
  int qindex = priority - 1;

  if((multiqueue.end[qindex] + 1) % (NPROC + 1) == multiqueue.start[qindex])
    return;
  
  multiqueue.proc[qindex][multiqueue.end[qindex]] = p;
  multiqueue.end[qindex] = (multiqueue.end[qindex] + 1) % (NPROC + 1);
}

static struct proc*
deque(int priority)
{
  struct proc *p;
  int qindex = priority - 1;

  if(multiqueue.start[qindex] == multiqueue.end[qindex])
    return 0;

  int index = multiqueue.start[qindex];

  p = multiqueue.proc[qindex][index];
  multiqueue.start[qindex] = (index + 1) % (NPROC + 1);

  return p;
}

/*
static void
print(void)
{
  for(int i = 0; i < 3; i++){
    int j = multiqueue.start[i];    
    while(j != multiqueue.end[i]){
      cprintf("%d ", multiqueue.proc[i][j]->pid);
      j = (j + 1) % (NPROC+1);
    }
    cprintf("\n");
  }
}
*/

#endif

#ifdef MLQ

static void swapque(int, int, int);

static void
swapque(int priority1, int priority2, int pid)
{
  struct proc *p = 0;
  int qindex1 = priority1 - 1;

  int i = multiqueue.start[qindex1];
  while(i != multiqueue.end[qindex1]){
    if(multiqueue.proc[qindex1][i]->pid == pid){
      p = multiqueue.proc[qindex1][i];

      int j = (i + 1) % (NPROC + 1);
      while(j != multiqueue.end[qindex1]){
        multiqueue.proc[qindex1][j - 1] = multiqueue.proc[qindex1][j];
        j = (j + 1) % (NPROC + 1); 
      }

      break;
    }

    i = (i + 1) % (NPROC + 1);
  }

  multiqueue.end[qindex1] = (multiqueue.end[qindex1] - 1) % (NPROC + 1);

  if(p)
    enque(priority2, p);
}

#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
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
  p->timecreated = ticks;
  p->timeready = 0;
  p->timerun = 0;
  p->timeslept = 0;
  p->switchnum = 0;

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

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

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

  #if defined(MLQ) || defined(DMLQ)
  p->priority = MDPR;
  #endif

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  #ifdef FCFS
  insert(p);
  #endif

  #if defined(MLQ) || defined(DMLQ)
  enque(p->priority, p);
  #endif

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
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
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;

  #if defined(MLQ) || defined(DMLQ)
  np->priority = curproc->priority;
  #endif

  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  #ifdef FCFS
  insert(np);
  #endif

  #if defined(MLQ) || defined(DMLQ)
  enque(np->priority, np);
  #endif

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
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
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
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
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    #ifdef DEFAULT

    struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->timescheduled = ticks;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }

    #endif

    #ifdef FCFS
    
    struct proc *p = remove();

    if(p){
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->timescheduled = ticks;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }

    #endif
    
    #if defined(MLQ) || defined(DMLQ)

    struct proc *p = 0;

    for(int pr = 1; pr <= NUMPR; pr++){
      if(!isempty(pr)){
        p = deque(pr);
        break;
      }
    }

    if(p){
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.

      // for printing the dequed process and all the queues
      // cprintf("\npid:%d at %d\n", p->pid, mycpu()->apicid);
      // print();
      
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->timescheduled = ticks;

      swtch(&(c->scheduler), p->context);
      switchkvm();
      
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }  
      
    #endif

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
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  p->switchnum++;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;

  #ifdef FCFS
  insert(myproc());
  #endif

  #if defined(MLQ) || defined(DMLQ)
  enque(myproc()->priority, myproc());
  #endif

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
  struct proc *p = myproc();
  
  if(p == 0)
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
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

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
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;

      #ifdef FCFS
      insert(p);
      #endif

      #ifdef DMLQ
      p->priority = HIPR;
      #endif
      
      #if defined(MLQ) || defined(DMLQ)
      enque(p->priority, p);
      #endif
    }
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
      if(p->state == SLEEPING){
        p->state = RUNNABLE;

        #ifdef FCFS
        insert(p);
        #endif        
        
        #ifdef DMLQ
        p->priority = HIPR;
        #endif        
        
        #if defined(MLQ) || defined(DMLQ)
        enque(p->priority, p);
        #endif
      }
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

void
ps(void)
{
  struct proc *p;
  
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNING || p->state == RUNNABLE || p->state == SLEEPING)
      cprintf("pid:%d  name:%s\n", p->pid, p->name);
  }
  release(&ptable.lock);
}

void
chpr(int pid, int priority)
{
  #ifdef MLQ

  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid)
      break;
  }

  int oldpriority = p->priority;

  if(oldpriority != priority){
    p->priority = priority;

    if(p->state == RUNNABLE)
      swapque(oldpriority, priority, p->pid);
  }

  release(&ptable.lock);

  #endif
}

// updates process times every clock tick
void
updateprocesstimes(void)
{
  struct proc *p;
  
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

    if(p->state == RUNNING)
      p->timerun++;
    else if(p->state == RUNNABLE)
      p->timeready++;
    else if(p->state == SLEEPING)
      p->timeslept++;
  }
  release(&ptable.lock);
}

// variant of wait()
// waits and returns statistics
int
waitnstats(int *timerun, int *timeready, int *timeslept)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        
        *timerun = p->timerun;
        *timeready = p->timeready;
        *timeslept = p->timeslept;

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
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}