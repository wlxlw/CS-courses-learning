# 任务要求

为xv6增加一个特征，在进程运行的时候，xv6能够周期性的向其发出警报。

需要设计`sigalarm(interval, handler)`系统调用，在进程调用`sigalarm(n, fn)`后，进程每隔n个tick，会调用一次fn函数。在fn函数返回之后，恢复进程的状态，继续执行剩余的程序。如果进程调用`signalarm(0, 0)`内核会停止周期性的调用。

# 分析

0. 程序P正常运行
1. 产生硬件中断
2. kernel/trap.c中的usertrap函数处理中断，**如果trap的类型是时钟中断**
   1. 对时钟中断计数
   2. 如果计数值等于interval，**调用位于用户空间的handler函数**

## **需要明确两个问题：**

1. 在usertrap函数是运行在**内核空间**下，如何才能访问**用户空间下**的handler函数？

   usertrap函数最后会调用usertrapret函数，usertrapret会函数会恢复myproc()->trapframe中的寄存器数据到CPU寄存器，我们只需要改变存储在myproc()->trapframe->epc中的值，就能够通过usertrapret将PC寄存器的值改为handler函数的地址。

2. 在handler函数运行过程中，**CPU寄存器中的值会被改写**，因此从handler函数返回程序P时，程序P的寄存器信息被改变了

   需要在调用handler函数之前保存寄存器状态，在调用handler函数之后恢复寄存器状态。

# 代码

1. 在Makefile中增加alarmtest.c

   ```c
   UPROGS=\        
   		$U/_alarmtest\
   ```

2. 在user/user.h中添加系统调用的声明

   ```c
   int sigalarm(int ticks, void (*handler)());
   int sigreturn(void);
   ```

3. 在user/usys.pl，kernel/syscall.h，kernel/syscall.c中注册两个系统调用

   ```c
   // in user/usys.pl
   entry("sigalarm");
   entry("sigreturn");
   
   //in kernel/syscall.h
   #define SYS_sigalarm 22
   #define SYS_sigreturn 23
   
   //in kernel/syscall.c
   extern uint64 sys_sigalarm(void);
   extern uint64 sys_sigreturn(void);
   static uint64 (*syscalls[])(void) = {
   [SYS_sigalarm] sys_sigalarm,
   [SYS_sigreturn] sys_sigreturn,
   };
   ```

4. 在kernel/proc.h的struct proc补充一些变量

   ```c
   struct proc {
     //lab Traps alarm
     int interval;//保存传入sigalarm的interval
     void (*handlers)(void);//保存传入sigalarm的handlers
     int tick_cnt;//tick的计数器
     int flg;//标记位，标记sigalarm是否正在执行
     struct trapframe tmp_trapframe;//用来保存程序的状态即各种寄存器的值
     //......
   };
   ```

5. 在kernel/sysproc.c中完成两个系统调用

   ```c
   uint64
   sys_sigreturn(void)
   {
     *(myproc()->trapframe) = myproc()->tmp_trapframe;//从tmp_trapframe恢复原始程序运行状态
     myproc()->flg=0;//标记sigreturn运行结束
     return 0;
   }
   
   uint64
   sys_sigalarm(void)
   {
     int ticks;
     uint64 st; 
     if(argint(0, &ticks) < 0 || argaddr(1, &st) < 0)//读取传入系统调用的两个参数
       return -1;
     myproc()->interval = ticks;//保持参数在pcb中
     myproc()->handlers = (void*)st;
     return 0;
   }
   ```

6. 在kernel/trap.c中补充

   ```c
   void
   usertrap(void)
   {
     //....
     if(r_scause() == 8){
     //....
     } else if((which_dev = devintr()) != 0){
     // ok
     } else {
       //...
     }
     if(p->killed)
       exit(-1);
   
   //lab Traps alarm
     if(which_dev==2){//如果是时钟中断
       p->tick_cnt++;//计数器加1
       if(p->tick_cnt == p->interval && p->flg==0){
         //如果计数值等于阈值，而且sigalarm没有正在执行
         p->tmp_trapframe = *(p->trapframe);//保存程序状态
         p->trapframe->epc =(uint64) p->handlers;//修改pc，让在userret之后跳转到handlers函数处玉兴
         p->tick_cnt = 0;//计数器归0
         p->flg = 1;//修改标记位,正在运行sigalarm
       }
     }
   //lab Traps alarm
    
     // give up the CPU if this is a timer interrupt.
     if(which_dev == 2)
       yield();
     usertrapret();
   }
   ```
