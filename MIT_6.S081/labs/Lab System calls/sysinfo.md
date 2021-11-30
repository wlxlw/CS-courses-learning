# **任务要求**

实现系统调用sysinfo，该系统调用读入指向struct sysinfo的指针p，并收集系统中空闲的内存**的数量**到p->freemen，收集state不是UNUSED的进程**的数量**到p->nproc。

# 知识点

## 学习如何进行内核到用户进程之间的通信，将内核下的内容拷贝到用户进程下

**阅读sys_fstat() (kernel/sysfile.c)**

学习使用argaddr读取传入的地址参数st。

```c
uint64
sys_fstat(void)
{
  struct file *f;
  uint64 st; // user pointer to struct stat
  if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
  //通过argaddr读取传入ftat系统调用的指针
    return -1;
  return filestat(f, st);
}
```

**阅读filestat系统调用 (kernel/file.c)**

学习copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)

在内核的src指向的地址处拷贝长度为len的内容到用户进程(的dstva地址下。(因为内核页表与用户进程页表不同，而程序运行在内核下，所以还需要提供用户进程的页表pagetable)

```c
// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;

  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}
```

## 学习如何访问CPU上的进程队列。

**推荐阅读kill kernel/proc.c**

```c
// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;
 //proc[NPROC]表示运行在当前CPU下的进程列表,通过访问proc列表，我们可以查看每一个进程的状态信息。
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}
```

**struct cpu**

```c
// Per-CPU state.
struct cpu {
  struct proc *proc; // The process running on this cpu, or null.
// 其他参数略去
};
```

**struct proc**

```c
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };//各种进程状态的定义
// Per-process state
struct proc {
 enum procstate state;        // Process state
 // 此处省略了其他状态信息
}
```

## 学习如何访问空间的内存空间

参考[MIT 6.S081 2020 Lab2 system calls讲解 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/332243456)

这一块实验指导书中没有明确说明。内核管理空闲空间的方法是**将空闲的空间一页一页的链接起来，形成一个空间页的链表。通过访问该链表，我们可以实现对空间空间的访问**

阅读kernel/kalloc.c。

我们可以通过`struct run *r = kmem.freelist;`来获得指向空闲页的指针。如果指针r不为空，则表示该页是空闲的。

# 实现

## 注册sysinfo

首先和实现trace系统调用一样，需要先注册sysinfo系统调用

- 在kernel/syscall.h中**补充**系统调用号`#define SYS_sysinfo 23`

- 在user/usys.pl中**补充**`entry("sysinfo");`

- 在kernel/syscall.c**补充** `extern uint64 sys_sysinfo(void);`

- 在kernel/syscall.c的syscalls[]函数指针数组中**补充**

  ```c
  static uint64 (*syscalls[])(void) = {
  //其他系统调用号与对应的函数指针
  [SYS_sysinfo] sys_sysinfo,
  }
  ```

## 读取sysinfo传入的参数

在kernel/sysproc.c中**补充**

```C
uint64
sys_sysinfo(void)//可以模仿sys_fstat() (kernel/sysfile.c)的写法
{
   uint64 addr;//地址是的类型是uint64
   if(argaddr(0, &addr) < 0)
     return -1;
   return sysinfo(addr);//调用kernel/proc.c中的sysinfo函数
}
```

别忘了在kernel/defs.h**补充**函数声明`int sysinfo(uint64);`

## 获取空闲空闲的字节数目

在kernel/kalloc.c中**补充**函数freecounter

```c
uint64
freecounter(void){
 struct run *r = kmem.freelist;//获取空闲页链表的头指针
 uint64 cnt = 0;
 while(r){ //计算空闲页的数量
   cnt+=1;
   r = r->next;
 }
 return cnt*PGSIZE;//返回空闲空间字节数，PGSIZE表示一页的字节数目
}
```

**别忘了在kernel/defs 中声明 **`uint64 freecounter(void)`

## 读取进程的数目&完成sysinfo系统调用的主要功能部分

在kernel/proc.c中补充**sysinfo函数**

```c
int sysinfo(uint64 addr){//可以模仿filestat系统调用(kernel/file.c)的写法
  struct proc*p;
  struct sysinfo tmp;
  tmp.nproc = 0;
  // 遍历运行在当前cpu下的所有进程,如果进程不是UNUSED，则计数
  for(p = proc; p < &proc[NPROC];p++){
      acquire(&p->lock);
      if(p->state != UNUSED){//如果进程的状态不是UNUSED，则计数
        tmp.nproc++;
      }
    release(&p->lock);
  }
  //对空闲空间计数
  tmp.freemem = freecounter();
  //保存tmp到用户空间
  p = myproc();
  if(copyout(p->pagetable, addr, (char*)&tmp, sizeof(tmp)) < 0)
      return -1;
  return 0;
}
```

