#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MSGSIZE 8
#define NULL 0

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct {
	struct spinlock lock;
	struct container container[NCONT];
} ctable;

static struct proc *initproc;
int scheduler_log = 0;

// @sunil Improve this later std =0 containerID change back after use
int allocFor = 0;
int scheduler_history = 0;
int memory_log = 0;
int memory_history =0;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

// Container Number current
int containerNum;

void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
	// Added by @sunil
	initlock(&ctable.lock, "ctable" );
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
allocproc()
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
  // @sunil @prabhat container ID stuff

  release(&ptable.lock);

  // Allocate kernel stack. @sunil get memory from container
  if(allocFor == 0){
	  if((p->kstack = kalloc()) == 0){
			p->state = UNUSED;
			return 0;
	  }
	}else{
		if((p->kstack = conalloc(allocFor,p->pid)) == 0){
			p->state = UNUSED;
			return 0;
		}
	}
  // reched end of stack downward 
  // growing
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
// Set up first user process. @sunil @prabhat
void
userinit(void)
{

  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  // Container Id belong to container 0
  p->containerID = 0;

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

    // Create First Container
  containerNum = 1;
  acquire(&ctable.lock);
  int itr=0,itr2=0;
  for(itr=0;itr<NCONT;itr++){
  	ctable.container[itr].state = BLANK;
  	ctable.container[itr].used = 0;
  	for(itr2=0;itr2<PROCESS_COUNT;itr2++){
  		ctable.container[itr].process[itr2] = NULL;
  		ctable.container[itr].schedulerHelper[itr2] = 0;
  	}
  	ctable.container[itr].done = 0;
  }

  ctable.container[0].state = WORKING;
  ctable.container[0].generatedProcess = 1;
  // here we know slot 0 will be free for sure @sunil
  ctable.container[0].process[0] = p;
  release(&ctable.lock);
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

  // Copy process state from proc. @sunil @prabhat
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
	kfree(np->kstack);
	np->kstack = 0;
	np->state = UNUSED;
	return -1;
  }

  // add to container table list and update list
  int fault = 1;
  if(curproc->containerID != 0){
	  acquire(&ctable.lock);
	  for(i=0;i<PROCESS_COUNT;i++){
	  	if(ctable.container[curproc->containerID].process[i]==NULL){
	  		if(curproc->containerID == 0){
	  			cprintf("Forked at id : %d\n",i);
	  		}
				ctable.container[curproc->containerID].process[i] = np;
				ctable.container[curproc->containerID].generatedProcess++;
	  		fault = 0;
	  		break;
	  	}
	  }
	  release(&ctable.lock);
}else{
	fault =0;
}
  if(fault==1){
  	kfree(np->kstack);
	np->kstack = 0;
	np->state = UNUSED;
	// @sunil @debug fast
	cprintf("Container - FULL  %d \n",curproc->containerID);
	return -2;
  }

  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  np->containerID = curproc->containerID;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

// @sunil @prabhat file stuff
  // try actual copy
  for(i = 0; i < NOFILE; i++)
	if(curproc->ofile[i])
	  np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

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

void ps_print(void){
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [WAITING]   "waiting",
  };
  struct proc *p;
  p = myproc();
  int containerID = p->containerID;
	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	  if(p->containerID != containerID)
	  	continue;

	  // if(p->state == RUNNING || p->state == WAIT )
		cprintf("pid:%d name:%s state: %s container : %d \n",p->pid,p->name,states[p->state],containerID);
	}
	release(&ptable.lock);
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
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int cntNum =0,itr=0,num=0,pid=-1,fault=0;
  	int curLocation[NCONT];

  	// Initialize
  	for(cntNum=0;cntNum<NCONT;cntNum++)
  		curLocation[cntNum]=0;

	for(;;){
		for(cntNum = 0;cntNum<containerNum;cntNum++ ){
			// Enable interrupts on this processor.
			sti();

			acquire(&ctable.lock);
			// cprintf("entered checkpoint for cnt : %d \n",cntNum);

			num=0;
			for(itr=curLocation[cntNum];num<PROCESS_COUNT;itr++){
				itr = itr%PROCESS_COUNT;
				if(ctable.container[cntNum].process[itr] != NULL){
					pid = ctable.container[cntNum].process[itr]->pid;
					curLocation[cntNum] = itr+1;
					if(scheduler_log){
						// cprintf("sheduled : %d  %d\n",pid,cntNum);
						ctable.container[cntNum].schedulerHelper[itr] = 1;
					}
					break;
				}
				num++;
			}
			// cprintf("exited checkpoint for cnt : %d  with pid %d  with num pro : %d loc : %d \n",cntNum,pid,ctable.container[cntNum].generatedProcess,curLocation[cntNum]);
			release(&ctable.lock);
			// Loop over process table looking for process to run.
			acquire(&ptable.lock);
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			  if(p->pid != pid && cntNum != 0)
			  	continue;

			  if(p->containerID != cntNum)
			  	continue;

			  // if( cntNum > 0)
			  	// p->state = RUNNABLE;

			  if(p->state != RUNNABLE)
				continue;

			  // Switch to chosen process.  It is the process's job
			  // to release ptable.lock and then reacquire it
			  // before jumping back to us.
			  c->proc = p;
			  switchuvm(p);
			  p->state = RUNNING;

			  // if( cntNum > 0 )
				  // cprintf("Container + %d : Scheduling process + %d \n",cntNum,pid);

			  swtch(&(c->scheduler), p->context);
			  switchkvm();

			  // Process is done running for now.
			  // It should have changed its p->state before coming back.
			  // if( cntNum > 0)
			  	// p->state = WAITING;
			  c->proc = 0;

			  // if( cntNum > 0 )
				  // cprintf("Container + %d : Leave Scheduling process + %d \n",cntNum,pid);

			}
			release(&ptable.lock);

			// check if scheduler has done it's job @sunil
			// ============================================
			if(scheduler_log){
				fault = 0; // 1 -> true 0-> false
				acquire(&ctable.lock);
				for(itr=0;itr<PROCESS_COUNT;itr++){
					if(ctable.container[cntNum].process[itr] != NULL){
						if(scheduler_log){
							if(ctable.container[cntNum].schedulerHelper[itr] != 1){
								fault = 1;
								break;
							}
						}
					}
				}
				if(fault==0){
					ctable.container[cntNum].done = 1;
				}
				release(&ctable.lock);
			}
		}
		if(scheduler_log){
			fault = 0;
			// check for all @sunil do it differently :p
			acquire(&ctable.lock);

			for(cntNum = 1;cntNum<containerNum;cntNum++ ){
				// cprintf("SCHEDULER TEST FOR : %d  and value %d \n",cntNum,ctable.container[cntNum].done);
				if(ctable.container[cntNum].done != 1){
					fault = 1;
					break;
				}

			}
			release(&ctable.lock);

			if(fault==0){
				//all done
				scheduler_log = 0;
				acquire(&ctable.lock);
				for(cntNum = 1;cntNum<containerNum;cntNum++ ){
					ctable.container[cntNum].done = 0;
					for(itr=0;itr<PROCESS_COUNT;itr++)
					ctable.container[cntNum].schedulerHelper[itr] = 0;
				}
				release(&ctable.lock);
			}
		}
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
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
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
  [ZOMBIE]    "zombie",
  [WAITING]   "waiting",
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



// struct QNode
// {
//     char* msg = (char *)malloc(MSGSIZE);
// };


// struct Queue
// {
//  // char* msg = (char *)malloc(MSGSIZE);;
//   struct QNode msgAr[20];
//   int cur;
//   int isFull;
// };
// Structure of a Node

// referecne https://www.geeksforgeeks.org/queue-set-2-linked-list-implementation/

struct Node { 
	char msg[MSGSIZE]; 
	struct Node* link; 
}; 
  
struct Queue { 
	struct Node *front, *rear;
	struct spinlock lock;
};

// Function to create Circular queue
void enQueue(struct Queue *q, char* msg)
{
	struct Node *temp = (struct Node*)kalloc();
	// Copy msg
	int loc = 0;
	while(loc < MSGSIZE){
	  temp->msg[loc] = msg[loc];
	  loc++;
	}
	// temp->data = value;
	if (q->front == NULL)
		q->front = temp;
	else
		q->rear->link = temp;

	q->rear = temp;
	q->rear->link = q->front;
}

// Function to delete element from Circular Queue
int deQueue(struct Queue *q,char* msg)
{
	if (q->front == NULL)
	{
		cprintf ("Queue is empty");
		return -1;
	}

	// If this is the last node to be deleted
	if (q->front == q->rear)
	{
		// value = q->front->data;
		int loc=0;
		while(loc<MSGSIZE){
		  msg[loc] = q->front->msg[loc];
		  loc++;
		}
		kfree((void*)q->front);
		q->front = NULL;
		q->rear = NULL;
	}
	else  // There are more than one nodes
	{
		struct Node *temp = q->front;
		// value = temp->data;
		int loc = 0;
		while(loc < MSGSIZE){
		  msg[loc] = temp->msg[loc];
		  loc++;
		}
		q->front = q->front->link;
		q->rear->link= q->front;
		kfree((void*)temp);
	}
   return 0;
}

// Queue for every Proc
struct Queue msgQueue[NPROC];
int request[NPROC];

// pid to ProcId
int pidToProcid(int pid){

	acquire(&ptable.lock);
	int loop = 0;
	for(loop =0;loop < NPROC;loop++){
	  if(ptable.proc[loop].pid == pid)
		break;
	}
	release(&ptable.lock);
	if(loop==NPROC)
	  return -1;
	else
	  return loop;
}

int proc_container(int pid){
	int loop =0;
	// Loop over process table looking for process to run.
	acquire(&ptable.lock);
	for(loop =0;loop < NPROC;loop++){
	  // cprintf("ITR : %d pid %d : %d\n",loop,ptable.proc[loop].pid,pid);
	  if(ptable.proc[loop].pid == pid){
		release(&ptable.lock);
		return ptable.proc[loop].containerID;
		// break;
	  }
	}
	release(&ptable.lock);
	// @sunil : should not happen
	return -1;
}

int proc_container_num(int pid){
	int loop =0,id=-1,val=0;;
	// Loop over process table looking for process to run.
	acquire(&ptable.lock);
	for(loop =0;loop < NPROC;loop++){
	  // cprintf("ITR : %d pid %d : %d\n",loop,ptable.proc[loop].pid,pid);
	  if(ptable.proc[loop].pid == pid){
		// release(&ptable.lock);
		id  = ptable.proc[loop].containerID;
		// break;
	  }
	}
	for(loop =0;loop < NPROC;loop++){
	  // cprintf("ITR : %d pid %d : %d\n",loop,ptable.proc[loop].pid,pid);
		if(ptable.proc[loop].containerID == id){
			val++;
		}
		if(ptable.proc[loop].pid == pid){
			release(&ptable.lock);
			return val;
		}

	}
	release(&ptable.lock);
	// @sunil : should not happen
	return -1;
}

void printall(){
	int loop =0,itr;
	  static char *states[] = {
	  [UNUSED]    "unused",
	  [EMBRYO]    "embryo",
	  [SLEEPING]  "sleep ",
	  [RUNNABLE]  "runble",
	  [RUNNING]   "run   ",
	  [ZOMBIE]    "zombie",
	  [WAITING]   "waiting",

	  };

	 int maxItr = 1;
	for(itr=0;itr < maxItr;itr++){
		acquire(&ptable.lock);
		for(loop =0;loop < NPROC;loop++){
		  if(ptable.proc[loop].state != UNUSED){
			// release(&ptable.lock);
			// return ptable.proc[loop].containerID;
			  cprintf("ITR : %d pid %d : %d :: %s\n",loop,ptable.proc[loop].pid,ptable.proc[loop].containerID,states[ptable.proc[loop].state]);
			// break;
		  }
		}
		release(&ptable.lock);
		cprintf("\n\n\n\n\n");
	}
}

int totaleContainers(){
  return containerNum;
}

void addContainer(){
  containerNum++;
  // sys_mkdir("container_"+containerNum-1);
  // char *argv[3];
  // exec("ls",argv);
  acquire(&ctable.lock);
  ctable.container[containerNum-1].generatedProcess = 0;
  // So i Guess here we get start of container Addr
  // ctable.container[containerNum-1].startAddr = getContainerMemory();
  // cprintf("Memory : %p  :-: %p \n",(ctable.container[containerNum-1].startAddr)+4096,ctable.container[containerNum-1].startAddr);
 	int itr;
 	for(itr=0;itr<MCONT;itr++){
 		ctable.container[containerNum-1].startAddr[itr] = kalloc();
 	}

  release(&ctable.lock);
}

char* conalloc(int containerID,int pid){
	acquire(&ctable.lock);
	// take free space
	int next = ctable.container[containerID].used;
	ctable.container[containerID].used++;
	char* mem = ctable.container[containerID].startAddr[next];
	// struct cmap 
	ctable.container[containerID].memory[next].virt = (void*)next;
	ctable.container[containerID].memory[next].phys_start = (uint)mem;
	ctable.container[containerID].memory[next].phys_end = (uint)(mem+4096);
	ctable.container[containerID].memory[next].pid = pid;
	release(&ctable.lock);

	cprintf("memory %x  of container : %d  with next : %d with pid : %d\n",mem,containerID,next,pid);
	return mem;
}

int joinContainer(int containerID){

	int i, pid,parentpid;
	struct proc *np;
	struct proc *curproc = myproc();

	parentpid = curproc->pid;

	// cprintf("Enter join\n");
	allocFor = containerID;

	// Allocate process.
	if((np = allocproc()) == 0){
		return -1;
	}

	allocFor = 0;

	// cprintf("ALLOTED\n");
	// Copy process state from proc. @sunil @prabhat
	if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
		kfree(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		return -1;
	}

  // add to container table list and update list
  int fault = 1;
  acquire(&ctable.lock);
  for(i=0;i<PROCESS_COUNT;i++){
  	if(ctable.container[containerID].process[i]==NULL){
		ctable.container[containerID].process[i] = np;
		ctable.container[containerID].generatedProcess++;
  		fault = 0;
  		break;
  	}
  }
  release(&ctable.lock);

  if(fault==1){
  	kfree(np->kstack);
	np->kstack = 0;
	np->state = UNUSED;
	cprintf("Container FULL\n");
	return -2;
  }

	np->sz = curproc->sz;
	np->parent = curproc;
	*np->tf = *curproc->tf;
	np->containerID = containerID;

	// Clear %eax so that fork returns 0 in the child.
	np->tf->eax = 0;

	// @sunil @prabhat file stuff
	// try actual copy
	for(i = 0; i < NOFILE; i++)
		if(curproc->ofile[i])
			np->ofile[i] = filedup(curproc->ofile[i]);

	np->cwd = idup(curproc->cwd);

	safestrcpy(np->name, curproc->name, sizeof(curproc->name));

	pid = np->pid;

	acquire(&ptable.lock);

	np->state =  RUNNABLE; //WAITING;

	release(&ptable.lock);

	// kill current process
	cprintf("KILLED pid : %d \n",curproc->pid);
	// kill(curproc->pid);
	// for(;;){
	// 	if(curproc->state == UNUSED){
	// 		break;
	// 	}
	// 	curproc->state = RUNNABLE;
	// }
	// if(parentpid != pid ){

	// Lets kill parent process for this fork

		acquire(&ctable.lock);
  	for(i=0;i<PROCESS_COUNT;i++){
  		if(ctable.container[curproc->containerID].process[i]->pid ==parentpid){
			ctable.container[curproc->containerID].process[i] = NULL;
			ctable.container[curproc->containerID].generatedProcess--;
  			fault = 0;
  			break;
  		}
  	}
  	release(&ctable.lock);
  	if(fault==1){
  		cprintf("COULDN'T kill parent ");
  	}

	exit();
	// }
	return pid;

}

void switch_scheduler_log(){
	scheduler_log = 1;
}

int containerProcessNum(int containerID){
	int val;
	acquire(&ctable.lock);

	val = ctable.container[containerID].generatedProcess;

	release(&ctable.lock);
	return val;
}

int check_schedule_log(int arg){
    if((arg == scheduler_log) || (arg == 1 && scheduler_history == 1))
        return 1;
    return 0;
}


void switch_memory_log(){
	memory_log = 1;
	memory_history++;
}

int check_memory_log(int arg){
    if(arg == 0 && memory_log == 0)
        return 1;
    if(arg == 1)
    		return memory_history;
    return 0;
}