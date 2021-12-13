# 加速系统调用

许多系统调用可以通过在**内核空间与用户空间之间共享内存**实现加速。因为这样做可以让用户进程直接读取内核空间的数据，省略了切换到内核空间的时间开销。

## 任务要求

优化xv6中getpid()系统调用的执行速度。在一个进程创建时，pid会被存储在内核虚拟地址USYSCALL所对于的物理页处。ugetpid直接访问USYSCALL虚拟地址下的数据获得pid。

```c
int
ugetpid(void)
{
  struct usyscall *u = (struct usyscall *)USYSCALL;
  return u->pid;
}
```

## 分析

在创建用户进程页表的时候(proc_pagetable函数中)，通过mappages函数通过用户进程页表建立虚拟地址USYSCALL到某一物理地址addr的映射，让用户进程能通过虚拟地址USYSCALL直接访问到存储在物理地址addr下的pid。与此同时，内核也能够访问到物理地址addr，以实现对物理地址addr下pid的修改。

阅读proc_pagetable函数与proc.h的过程中，发现p->trapframe同时可以被用户进程以及内核访问，因此可以模仿这一写法实现任务要求

## 实现

1. 在proc.h中定义usyspage用来存放一个物理地址。用户进程页表可以映射到这个物理地址，内核可以访问并修改该物理地址下的内容。

   ```c
   #define TRAPFRAME (TRAMPOLINE - PGSIZE)
   #define USYSCALL (TRAPFRAME - PGSIZE)
   struct proc {
    //其他参数
    struct usyscall * usyspage;
   };
   ```

2. 修改proc_pagetable函数，修改用户进程页表，使得用户进程的虚拟地址USYSCALL映射到p->usyspage地址下

   ```c
   pagetable_t
   proc_pagetable(struct proc *p)
   {
     //其它代码省略
     if(mappages(pagetable, USYSCALL, PGSIZE,
                 (uint64)(p->usyspage), PTE_U|PTE_R) < 0){
       uvmfree(pagetable, 0);
       return 0;
     }
     //其它代码省略
   }
   ```

3. 修改allocproc函数，在修改用户进程页表(步骤2)之前，内核首先使用kalloc函数从空闲页表中取出一个空闲的物理页，将物理页的起始地址赋值给p->usyspage，并将进程的pid保存再这个物理地址下

   ```c
   static struct proc*
   allocproc(void)
   {
     p->pid = allocpid();
    //其它代码省略
     if((p->usyspage =  (struct usyscall *)kalloc())==0)   {
       freeproc(p);
       release(&p->lock);
       return 0;
     }
     p->usyspage->pid = p->pid;
    //其它代码省略
     p->pagetable = proc_pagetable(p);
   }
   ```

4. 释放内存空间

   ```c
   static void
   freeproc(struct proc *p)
   {
    //其它代码省略
     if(p->usyspage)
       kfree((void*)p->usyspage);
    //其它代码省略
   }
   
   void
   proc_freepagetable(pagetable_t pagetable, uint64 sz)
   {
    //其它代码省略
     uvmunmap(pagetable, USYSCALL, 1, 0);
    //其它代码省略
   }
   ```








