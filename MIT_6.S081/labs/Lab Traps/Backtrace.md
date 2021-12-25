# 了解RICS-V指令、寄存器、栈、栈帧

**究极好文：** [理解应用程序和执行环境 — rCore-Tutorial-Book-v3 0.1 文档 (rcore-os.github.io)](https://rcore-os.github.io/rCore-Tutorial-Book-v3/chapter1/4understand-prog.html)

## 为什么要每个进程都要弄个栈出来？

程序的执行通常离不开**函数调用**：

1. 程序调用函数f
2. 跳转到函数f
3. 回到程序原本位置，继续执行程序

可以看到在执行完函数调用后，程序需要返回并继续执行。而在执行函数调用的过程中，会改变CPU的部分寄存器(例如pc)。由于CPU只有一组寄存器，为了恢复调用函数过程中被污染寄存器，需要提前保存寄存器的值到内存中，因此**设计了栈**来**保存函数调用上下文**

## RISC-V stack

1. RISC-V栈从高地址向低地址处增长
2. 栈占一页的物理空间
3. sp寄存器保存栈顶地址

## RISC-V stack frame

在一个函数中，会有一段开场代码向栈申请一块空间，即` addi    sp,sp,-16`减少sp寄存器的值，于是**[新sp,旧sp)**对应的物理空间就用来进行函数调用上下文的保存和恢复。这块物理空间也被称为栈帧。在函数结尾时也会有一段代码用来回收栈帧`addi    sp,sp,16`增加sp寄存器的值

栈帧的部分结构

1. fp-8保存"函数返回地址"所在的地址
2. fp-16保存"前一个栈帧"的所在的地址

# 任务要求

实现backtrace，追踪函数的调用过程

# 代码

在程序调用函数的过程中，被调用函数的信息会被压入栈中。通过访问栈中存在的每一个栈帧，输出栈帧中存放的返回地址。

先按照题目要求在kernel/riscv.h中添加获得当前栈帧地址的函数

```c
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}
```

在kernel/printf.c中补充函数

```c
void
backtrace(){
  uint64 fp = r_fp();
  uint64 s_begin = PGROUNDUP(fp);//栈的起始地址
  uint64 s_end = PGROUNDDOWN(fp);//栈的结束地址
  //printf("begin %d end %d\n",s_begin,s_end);

  uint64* ra = (void*)(fp-8);//获得栈中存放的返回地址
  uint64* new_fp;
  while(fp<s_begin && fp>=s_end){//如果fp在栈中
    printf("%p\n",*ra);
    //printf("%p %p %p\n",fp,s_begin,s_end);
    new_fp = (void*)(fp-16);//更新栈帧的地址
    fp = *new_fp;
    ra = (void*)(fp-8);//更新返回地址
  }
}
```

在kernel/defs.h中补充函数申明

```c
void            backtrace(void);
```