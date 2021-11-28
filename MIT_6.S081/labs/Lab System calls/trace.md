**任务要求**

实现trace系统调用，追踪某一程序执行过程中调用的系统调用。trace系统调用读入mask(int 类型)，如果mask中第x个bit为1，则追踪系统调用号为x的系统调用。

**实现**

一个系统调用分为两个部分：1. 用户程序**调用**系统调用 2. 内核**执行**系统调用

1. 用户程序**调用**trace系统调用的代码

   查看user/trace.c，用户程序在user/trace.c中调用trace，即`trace(atoi(argv[1])` 。我们需要在user/user.h中**补充**trace系统调用的声明`int trace(int);` 。而调用trace的代码内容由user/usys.pl生成，我们需要在user/usys.pl中**补充**`entry("trace");`。

   最终用户程序执行

    ```assembly
   li a7, SYS_trace # 保存trace系统调用号到a7
   ecall  # trap(保存上下文，执行系统调用，恢复上下文，返回)
   ret
    ```

   trace的系统调用号SYS_trace需要补充在kernel/syscall.h中，`#define SYS_trace  22`。

2. 内核**执行**trace系统调用的代码

   查看kernel/syscall.c，内核首先从a7读取系统调用号到num，`num = p->trapframe->a7;`。然后执行对应的系统调用，`p->trapframe->a0 = syscalls[num]();`。而`syscall[num]` 是指向一个系统调用函数的指针。



   我们需要先**补充**kernel/proc.h中struct proc结构体的代码，这一数据结构保存了用户进程的状态信息。我们在struct proc中定义一个变量 trace_num用来保存trace系统调用读入的mask					

   ```c
   struct proc{
   //other variable
      int trace_num;//保存trace读入的mask
   }
   ```

   在kernel/sysproc.c中**补充**sys_trace系统调用函数的代码

   ```c
   uint64
   sys_trace(void)
   {
     struct proc *p = myproc();//指向用户进程状态的指针
     if(argint(0, &(p->trace_num)) < 0)//读取mask到struct proc中的trace_num
       return -1;
     return 0;
   }
   ```

    

   在kernel/syscall.c的fork()中**补充**`np->trace_num = p->trace_num;`，保存父进程的trace_num到子进程，从而使得子进程的系统调用也能被追踪。



   最后在kernel/syscall.c中**补充**

      1. 在syscalls系统调用函数指针数组中添加

   ```c
   static uint64 (*syscalls[])(void) = {
   //other syscall
   [SYS_trace]   sys_trace,
   };
   ```

   2. 添加从系统调用号到系统调用名称的映射数组

   ```c
   static char* syscalls_names[]={
   [SYS_fork]    "fork",
   [SYS_exit]    "exit",
   [SYS_wait]    "wait",
   [SYS_pipe]    "pipe",
   [SYS_read]    "read",
   [SYS_kill]    "kill",
   [SYS_exec]    "exec",
   [SYS_fstat]   "fstat",
   [SYS_chdir]   "chdir",
   [SYS_dup]     "dup",
   [SYS_getpid]  "getpid",
   [SYS_sbrk]    "sbrk",
   [SYS_sleep]   "sleep",
   [SYS_uptime]  "uptime",
   [SYS_open]    "open",
   [SYS_write]   "write",
   [SYS_mknod]   "mknod",
   [SYS_unlink]  "unlink",
   [SYS_link]    "link",
   [SYS_mkdir]   "mkdir",
   [SYS_close]   "close",
   [SYS_trace]   "trace",
   };
   ```

   3. 最后在kerne/syscall.c的syscall()中添加

   ```c
   void
   syscall(void)
   {
     int num;
     struct proc *p = myproc();
     num = p->trapframe->a7;
     if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
       p->trapframe->a0 = syscalls[num]();
       // trace 输出文件进程id，系统调用名，返回值
       if(((p->trace_num)&(1<<num)))
         printf("%d: syscall %s -> %d\n",p->pid,syscalls_names[num],p->trapframe->a0);
       //trace
   
     } else {
       printf("%d %s: unknown sys call %d\n",
               p->pid, p->name, num);
       p->trapframe->a0 = -1;
     }
   }
   ```

-----



**补充部分: ecall指令做了什么？**

4.2 Trap from user space

在执行系统调用ecall / 执行非法操作 / 设备中断时，**会发生trap。**

trap包含四个步骤：

1. 执行uservec

   1. 保存用户寄存器的信息到进程的rapframe中

   2. 将页表寄存器satp**从指向用户进程页表->到指向内核页表**

      (难点: 切换页表之后，仍要继续执行uservec程序。xv的做法是内核页表和用户页表都保留同一个页表项，从同一个虚拟地址到同一个保留了uservec程序的页。)

   3. 调用usertrap

2. 执行usertrap

   对各种产生tarp的原因，调用不同的程序(通过修改pc寄存器)来进行处理

   1. 如果是系统调用产生的，syscall
   2. 如果是设备终端产生的，devintr
   3. 如果是一个exception，kill出错的进程

3. 执行usertrapretet
   1. 恢复control registers
   2. 调用userret
4. 执行userret
   1. 将页表寄存器satp**从指向内核->到指向用户进程**
   2. 从trapframe中恢复用户寄存器的信息


