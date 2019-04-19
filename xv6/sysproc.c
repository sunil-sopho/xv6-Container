#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "syscall.h"

int Trace=0;

int
sys_fork(void)
{
  // count[SYS_fork-1]++;
  return fork();
}

int
sys_exit(void)
{
  // count[SYS_exit-1]++;
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
    // count[SYS_wait-1]++;
  return wait();
}

int
sys_kill(void)
{
  int pid;
  // count[SYS_kill-1]++;
  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
    // count[SYS_getpid-1]++;
  return myproc()->pid;
}

int
sys_sbrk(void)
{
    // count[SYS_sbrk-1]++;
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
    // count[SYS_sleep-1]++;
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
    // count[SYS_uptime-1]++;
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_toggle(void){
    // Toggles trace variable
    //cprintf("tringg to toggle %d\n",Trace);
    Trace = 1 - Trace;
    return 0;

}

int sys_print_count(void){

   

 //   ar[23] = "sys_exit";
    if(Trace == 0)
        return 0;

    countPrint();
    cprintf("print_count");
    // count[SYS_print_count-1]++;
    return 0;
}

int sys_add(void){
    // count[SYS_add-1]++;
    int num1,num2;
    if(argint(0,&num1)<0)
        return -1;
    if(argint(1,&num2)<0)
        return -2;
    return (num1+num2);
}


int sys_ps(void){
  // count[SYS_ps-1]++;
  ps_print();
  return 0; 
}

int sys_send(void){
  int sender_pid,rec_pid;
  char* msg;
  if(argint(0,&sender_pid)<0 || argint(1,&rec_pid) < 0 || argptr(2,&msg,8) < 0)
    return -1;
  int error  = sending(rec_pid,msg);
  // cprintf("sender: %d rec: %d \n",sender_pid,rec_pid);
  return error;
}

int sys_recv(void){
  // if(argptr(2,))
  char* msg;
  if( argptr(0,&msg,8) < 0){
    return -1;
  }
  int error = recv(msg);
  // cprintf("returnin with msg\n");
  return error;
}

int sys_send_multi(void){
  int sender_pid,num;
  char *msg;
  if(argint(0,&sender_pid)< 0 || argint(3,&num)<0 || argptr(2,&msg,8)<0){
    return -1;
  }
  int *ar;
  if(argintar(1,&ar,num)<0){
    cprintf("array input problem\n");
    return -1;
  }

  int error = send_multi(ar,msg,num);
  return error;

}


// source https://wiki.osdev.org/Shutdown
int
sys_halt(void)
{
  outb(0xf4, 0x00);
  return 0;
}