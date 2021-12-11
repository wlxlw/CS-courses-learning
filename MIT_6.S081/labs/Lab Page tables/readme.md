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

页表项(64位)          保留位(10) |  物理页号(44位)  |  标记位(10位)

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

# 代码：创建地址空间

xv6管理地址空间和页表的核心数据结构是pagetable_t(kernem/vm.c)，其中有两个核心函数(虚拟地址到物理地址的映射采用**直接映射**)：

1. walk函数，找到一个虚拟地址对应的页表项(PTE)

   1. 地址转换相关的的宏

   ```c
   #define PXMASK          0x1FF // 9 bits
   #define PXSHIFT(level)  (PGSHIFT+(9*(level)))
   #define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK)
   //PX(2, va) 保留虚拟地址va页号的高9位
   //PX(1, va) 保留虚拟地址va页号的中间9位
   //PX(0, va) 保留虚拟地址va页号的低9位
   #define PTE2PA(pte) (((pte) >> 10) << 12)
   //页表项往右移10位获得物理页号，物理页号再左移12位获得该页*起始位置*的物理地址
   #define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)
   #define PTE_V (1L << 0) // valid
   typedef uint64 pte_t; //pte_t为64位虚拟地址
   typedef uint64* pagetable_t;
   ```

   2. walk函数代码解释

   ```c
   pte_t *
   walk(pagetable_t pagetable, uint64 va, int alloc)
   {
     if(va >= MAXVA)
       panic("walk");
     //找到虚拟地址va在多级页表对应的页表项
     for(int level = 2; level > 0; level--) {
       pte_t *pte = &pagetable[PX(level, va)];//获得当前层虚拟地址对应的页表项
       if(*pte & PTE_V) {
         pagetable = (pagetable_t)PTE2PA(*pte);//根据页表项获得对应物理页的起始地址
       } else {//如果不存在
         if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)//没有空间可以分配返回0
           return 0;
         memset(pagetable, 0, PGSIZE);//为该页分配内存
         *pte = PA2PTE(pagetable) | PTE_V;//修改页表项的物理地址以及有效位
       }
     }
     return &pagetable[PX(0, va)];//返回指向va对应页表项的指针
   }
   ```

   

2. mappages函数，将给定的虚拟地址段映射到给定的物理地址段(通过修改页表项)

   1. 相关宏

   ```c
   #define PGSIZE 4096 // bytes per page
   #define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
   //PGSIZE-1 = 0xFFF
   //PGROUNDDOWN(a) 返回虚拟/物理地址a所在页的起始地址
   ```
   2. mappages函数

   ```c
   // Create PTEs for virtual addresses starting at va that refer to
   // physical addresses starting at pa. va and size might not
   // be page-aligned. Returns 0 on success, -1 if walk() couldn't
   // allocate a needed page-table page.
   int
   mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
   {
     uint64 a, last;
     pte_t *pte;
   
     if(size == 0)
       panic("mappages: size");
   //将虚拟地址段[va,va+size-1]映射到物理地址段[pa,pa+size-1]
     a = PGROUNDDOWN(va);//找到虚拟地址va对应虚拟页的起始地址
     last = PGROUNDDOWN(va + size - 1);
     for(;;){
       if((pte = walk(pagetable, a, 1)) == 0)
         return -1;
       if(*pte & PTE_V)//对应页表项已经存在，不能重新映射到物理地址pa
         panic("mappages: remap");
       *pte = PA2PTE(pa) | perm | PTE_V;//修改页表项，完成地址映射
       if(a == last)
         break;
       a += PGSIZE; //遍历下一页
       pa += PGSIZE;
     }
     return 0;
   }
   ```

**内核页表如何创建**

1. boot调用kvminit函数
2. kvminit函数调用kvmmake函数
3. kvmmake函数
   1. 首先为内核的根页表分配一个物理页
   2. 然后调用kvmmap生成虚拟地址到物理地址的映射，让内核能够知道指令以及数据所在的位置
   3. kvmmap函数通过调用mappages函数来创建虚拟地址到物理地址之间的映射
4. 调用kvminithart函数
   1. 将根页表的物理地址写入satp寄存器
   2. 清空TLB缓存

# 物理内存分配

xv6分配或者回收一页的内存，物理内存分配器采用free list数据结构来分配物理页。free list的每一个元素是一个run数据结构。物理内存分配器直接将每一个run数据结构保存在空闲的页中。

```c
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
```

**kinit函数初始化物理内存分配器**

1. 生成lock

2. 调用freerage函数为freelist分配物理页

3. 调用kfree函数头插法创建空闲物理页链表

   ```c
   // Free the page of physical memory pointed at by v,
   // which normally should have been returned by a
   // call to kalloc().  (The exception is when
   // initializing the allocator; see kinit above.)
   void
   kfree(void *pa)
   {
     struct run *r;
   
     if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
       panic("kfree");
   
     // Fill with junk to catch dangling refs.
     memset(pa, 1, PGSIZE);
   //r指向物理地址pa, *r访问pa所在物理页
     r = (struct run*)pa;
   //kmem.freelist指向空闲物理页链表的头部
   //头插法建立空闲物理页表
     acquire(&kmem.lock);
     r->next = kmem.freelist;
     kmem.freelist = r;
     release(&kmem.lock);
   }
   ```

   

