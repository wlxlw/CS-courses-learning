# 陷入与系统调用

三种中断CPU的事件：

1. 系统调用，用户程序执行ecall
2. 异常，程序执行错误
3. 中断，设备发出中断

本书使用trap来通称这三种情况，我们通常希望trap是透明的，即程序在执行过程中不会意识到trap的发生。trap通常会执行以下几个步骤：

1. 移交控制权到内核
2. 内核保存寄存器和程序执行时的状态
3. 内核执行相应的处理程序
4. 内核恢复寄存器和程序执行时的状态

xv6在内核中处理所有trap，trap处理程序分四个步骤执行：

1. CPU硬件部分的操作
2. 准备内核C程序的相关汇编代码
3. 与trap相关的C程序
4. 与系统调用和设备去当相关的程序

由于trap有三种类型(系统调用、异常、中断)，trap程序分为3块：

1. trap from user space
2. trap from kernel space
3. timer interrupts

内核中处理trap的代码通常被称作handler

# RISC-V trap机制

RISC-V CPU设置来许多控制寄存器来告诉CPU如何处理trap，kernel/riscv.h中定义了xv6使用的寄存器：

1. stvec：内核在此写入trap handler的地址，RISC-V跳转到stvec地址处来处理trap
2. sepc：当trap发生时，RISC-V在此保存程序计数器，sret(return from trap)指令复制sepc到pc。内核可以通过修改sepc的值来控制sret的流向
3. scause：RISC-V在此设置一个数字用来标记发生的trap的类型
4. sscratch：内核此放置一个值，方便trap handler的执行
5. sstatus：sstatus的SIE bit控制设备是否允许中断,SIE为0不允许中断，SPP bit标记trap是从用户模式放回还是内核模式返回

以上的寄存器只在内核模式下允许被读写。

trap(除了timer interrupts)发生时，**RISC-V硬件**会完成以下操作：

1. 如果trap是设备中断，如果sstatus的SIE bit为0，则结束
2.  清零sstatus的SIE bit，不允许多重中断
3. 复制pc到sepc，保留原始程序的pc
4. 保存当前的系统权限(内核模式、用户模式)到sstatus的SPP bit
5. 设置scause为发生的trap的类型
6. 系统权限进入内核模式
7. 保存stvec到pc
8. 从pc开始执行trap处理程序

**需要注意的是，硬件没有完成**

1. **切换到内核页表，**
2. **切换到内核栈，**
3. **保存除pc外的其它寄存器**

**以上三种操作都需要软件来完成**。在处理trap时让CPU完成最小所需操作设计的原因是，给软件留下足够大的灵活性，例如操作系统在某些情况下可以不切换页表实现对trap处理的加速。

# Trap from user space

xv6用不同的方式处理的来自内核或者用户空间的trap，本节描述来自用户空间的trap。

三种来自于用户空间的trap：

1. 用户进程通过ecall指令调用系统调用
2. 非法操作
3. 设备中断

xv6 trap handle设计的主要限制：

RISC-V硬件在处理trap的时候不切换页表，这意味着trap handler需要实现切换页表的操作，并且需要保证及时切换了页表，trap handler也能正常的运行

xv6使用trampoline page来实现这个需求。trampoline page被映射在**每一个用户进程页表**以及**内核页表**的TRAMPOLINE地址下。因为trampoline page映射在不同页表的同一个虚拟地址下，即使从用户进程页表切换到内核页表，trap handler也可以正常地执行程序。

**trap handler**

uservec函数：保存用户进程的状态，切换到内核页表

1. 交换a0和sscratch寄存器的值。a0本来存放用户进程的某个状态，sscratch本来存放用户进程trapframe的地址。交换两个寄存器的值后，uservec保存用户寄存器的值到trapframe (**在用户进程页表下运行**)
2. 恢复sp为内核栈指针，恢复当前cpu的hartid，**恢复satp为内核页表**。需要恢复的这3个数据都保存在tramframe中。（**进入内核页表下运行**)
3. 调用usertrap，因为trapframe中还保存了usertrap函数的地址

usertrap函数：确定trap的成因，处理trap并返回

1. 修改stvec的值为kernelvec的地址。因为此时程序运行在内核下，如果再次发生trap需要调用kernelvec来处理。
2. 保存sepc寄存器(用户程序pc)的值。因为在内核中运行时也可能会出现trap
3. 如果trap时
   1. 系统调用，调用syscall
   2. 设备中断，调用devintr
   3. 其它，内核直接kill错误进程
4. 调用usertrapret

usertrapret函数：初始化控制寄存器的值以准备下一次来自用户空间的trap

1. 修改stvec的值为uservec函数的地址
2. 准备下一次运行uservec所需要的trapframe，设置内核页表地址，内核栈地，cpu hartid。
3. 恢复sepc为用户进程的pc
4. 调用userret

userret函数：

1. 切换到进程页表。设置satp为用户进程的页表 (userret程序仍可正常执行，因为userret位于trampoline page中，内核与用户进程保留相同的映射)
2. 将a0(TRAPFRAME)保存会ssratch，以备下一次tarp
3. 恢复进程状态。从trapframe中恢复用户寄存器的值
4. 执行sret返回用户空间，修改程序计数器sepc和系统权限sstatus

# 内核如何与用户进程交互数据

copyinstr函数能够将用户页表pagetable虚拟地址srcva处的数据转移到内核的dst地址下。通过

1. 调用walkaddr函数获取srcva对应的页表项，从而获得物理地址pa。walkaddr函数通过检查srcva是否异常用户进程地址空间，以及页表项的PTE_U和PTE_V确认是否时合法的地址
2. 因为内核页表虚拟地址直接映射到物理地址，直接复制pa下的数据到dst处，从而保存数据到内核。

copyout以相似的原理实现复制内核下的数据到用户进程地址空间下。

# Traps from kernel space

当内核在CPU上运行时，stvec指向kernelvec程序所在地址

kernelvec：

1. 保存寄存器的值到被中断线程的栈
2. 跳转到kerneltrap函数
   1. 保存sepc和sstatus
   2. 判断trap的类型
      1. 设备中断，调用devintr判断是否是设备中断，并处理
      2. 异常。说明有重大错误发生，xv6停止执行
      3. timer interrupt，调用yield放弃CPU
   3. 恢复sepc和status。因为yield后可能有其它线程污染了sepc和status
3. 恢复寄存器的值
4. 执行sret，复制sepc到pc，以恢复到被中断的内核代码

# Page-fault exceptions

对任何异常，xv6直接kill掉产生异常的程序

