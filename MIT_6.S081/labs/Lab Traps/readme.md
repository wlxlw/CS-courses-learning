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

1. 如果trap时设备中断，如果sstatus的SIE bit为0，则结束
2.  清零sstatus的SIE bit，不允许多重中断
3. 复制pc到sepc，保留原始程序的pc
4. 保存当前的系统权限(内核模式、用户模式)到sstatus的SPP bit
5. 设置scause为发生的trap的类型
6. 进入内核模式
7. 保存stvec到pc
8. 从pc开始执行trap处理程序

**需要注意的是，硬件没有完成切换到内核页表，没有完成切换到内核栈，没有完成保存除pc外的其它寄存器。这些操作都需要软件来完成。**在处理trap时让CPU完成最小所需操作设计的原因是，给软件留下足够大的灵活性，例如操作系统在某些情况下可以不切换页表实现对trap处理的加速。

# Trap from user space

xv6用不同的方式处理的来自内核或者用户空间的trap，本节描述来自用户空间的trap











