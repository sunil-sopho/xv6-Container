#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"


int count[TotaleSysNum];

void clearCount(){
   int num = TotaleSysNum,i=0;
      for(i=0;i<num;i++){
        count[i] = 0;
    }
}

void countPrint(){
  const char *ar[TotaleSysNum];
ar[0] = "sys_fork";
ar[1] = "sys_exit";
ar[2] = "sys_wait";
ar[3] = "sys_pipe";
ar[4] = "sys_read";
ar[5] = "sys_kill";
ar[6] = "sys_exec";
ar[7] = "sys_fstat";
ar[8] = "sys_chdir";
ar[9] = "sys_dup";
ar[10] = "sys_getpid";
ar[11] = "sys_sbrk";
ar[12] = "sys_sleep";
ar[13] = "sys_uptime";
ar[14] = "sys_open";
ar[15] = "sys_write";
ar[16] = "sys_mknod";
ar[17] = "sys_unlink";
ar[18] = "sys_link";
ar[19] = "sys_mkdir";
ar[20] = "sys_close";
ar[21] = "sys_toggle";
ar[22] = "sys_print_count";
ar[23] = "sys_add";
ar[24] = "sys_ps";
ar[25] = "sys_halt";
ar[26] = "sys_create_container";
ar[27] = "sys_destroy_container";
ar[28] = "sys_join_container";
ar[29] = "sys_leave_container";
ar[30] = "sys_proc_container";
ar[31] = "sys_proc_container_num";
ar[32] = "sys_scheduler_log_on";
ar[33] = "sys_containerProcessNum";
ar[34] = "sys_check_schedule_log";
ar[35] = "sys_memory_log_on";
ar[36] = "sys_check_memory_log";

 int num = TotaleSysNum,i=0;
      for(i=0;i<num;i++){
        if(count[i]!=0)
        cprintf("%s %d\n",ar[i],count[i]);
    }
}

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  struct proc *curproc = myproc();

  if(addr >= curproc->sz || addr+4 > curproc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  struct proc *curproc = myproc();

  if(addr >= curproc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)curproc->sz;
  for(s = *pp; s < ep; s++){
    if(*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;
  struct proc *curproc = myproc();
 
  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

int argintar(int n,int **ar,int size){
  int i;
  struct proc *curproc = myproc();
  if(argint(n,&i)<0)
    return -1;
  if(size <0 ||(uint)i >= curproc->sz || (uint)i+size > curproc->sz)
    return -1;
  *ar = (int*)i;
  return 0;

}


extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_toggle(void);
extern int sys_print_count(void);
extern int sys_add(void);
extern int sys_ps(void);
extern int sys_halt(void);
extern int sys_create_container(void);
extern int sys_destroy_container(void);
extern int sys_join_container(void);
extern int sys_leave_container(void);
extern int sys_proc_container(void);
extern int sys_proc_container_num(void);
extern int sys_scheduler_log_on(void);
extern int sys_containerProcessNum(void);
extern int sys_check_schedule_log(void);
extern int sys_memory_log_on(void);
extern int sys_check_memory_log(void);
extern int sys_file_creation_log(void);
extern int sys_check_file_creation(void);
extern int sys_isAllEnded(void);

static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_toggle]   sys_toggle,
[SYS_print_count]   sys_print_count,
[SYS_add] sys_add,
[SYS_ps] sys_ps,
[SYS_halt] sys_halt,
[SYS_create_container] sys_create_container,
[SYS_destroy_container] sys_destroy_container,
[SYS_join_container] sys_join_container,
[SYS_leave_container] sys_leave_container,
[SYS_proc_container] sys_proc_container,
[SYS_proc_container_num] sys_proc_container_num,
[SYS_scheduler_log_on] sys_scheduler_log_on,
[SYS_containerProcessNum] sys_containerProcessNum,
[SYS_check_schedule_log] sys_check_schedule_log,
[SYS_memory_log_on] sys_memory_log_on,
[SYS_check_memory_log] sys_check_memory_log,
[SYS_file_creation_log] sys_file_creation_log,
[SYS_check_file_creation] sys_check_file_creation,
[SYS_isAllEnded] sys_isAllEnded
};

void
syscall(void)
{
  int num;
  struct proc *curproc = myproc();

  num = curproc->tf->eax;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    count[num-1]++;
    curproc->tf->eax = syscalls[num]();
    if(num==22)
      clearCount();
    // cprintf("%d  %s\n",num,syscalls[num]);
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}
