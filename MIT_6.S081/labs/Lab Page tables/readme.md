# 页表硬件

**Sv39 RISC-V支持39位虚拟地址空间**

**RISC-V页表的结构**

1. 有2^27^ 个页表项（Page Table Entries PTE）。
2. 每个页表项包含44位的物理页号（Physical Page Number PPN）
3. 每个页表项包含一些标记位（定义在kernel/riscv.h）
   1. PTE_V 存在位
   2. PTE_R 是否可读
   3. PTE_W 是否可写
   4. PTE_X  是否可以执行
   5. PTE_U 是否允许在用户模式下访问

**地址映射**

虚拟地址(39位)      页号(27位) | 页内偏移(12位)

物理地址(56位)      页号(44位) | 页内偏移(12位)

**多级页表**

RISC-V使用3级页表进行地址映射，并使用快表(TLB)加速地址转换速度。

**页表基址寄存器**

根页表的起始地址保存在satp寄存器中。每个CPU都有自己的satp寄存器，因此不同的CPU可以运行不同的程序。

**内核如何修改页表**

因为内核有访问修改内存的权限，而页表保存在内存中，所以内核自然可以修改页表

# 内核地址空间

xv6给每一个进程维护一个页表，用来描述用户地址空间。并用一个额外的页表描述内核地址空间。

**那么内核怎么知道通过页表访问硬件(内存以及其它I/O设备)呢？**

I/O设备(例如磁盘)被映射到某个物理内存地址下(memory-mapped control registers)，内核通过访问这一块内存来与外设交互。

**问题转换成内核怎么访问内存？**

内核地址空间的页表**大部分**采用**直接映射**(direc mapping)的策略，将虚拟地址直接作为物理地址，这样内核就能直接通过页表获得内存的真实地址，而不用经过地址映射。

**不采用直接映射的虚拟地址空间**

1. The trampoline page放在虚拟地址空间的最高地址下，内核页表与用户页表保留同样的The trampoline page地址映射。

2. guard page，guard page的物理地址在内存中，虚拟地址在每一个内核栈的相邻的高位地址下，用来防止每个进程的内核栈的溢出。

   ​	内核栈和用户栈是什么?  [from xv6 book 2.5 Process overview](When the process is
   executing user instructions, only its user stack is in use, and its kernel stack is empty. When the
   process enters the kernel (for a system call or interrupt), the kernel code executes on the process's
   kernel stack; while a process is in the kernel, its user stack still contains saved data, but isn't actively
   used.)

   ​			每个用户进程都有自己的用户栈和内核栈，在用户进程运行用户程序	的时候，只有用户栈被使用；在用户进程进入内核后(通过系统调用或者	中断)，内核在内核栈中运行程序。

