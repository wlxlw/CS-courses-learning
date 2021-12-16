# 任务要求

实现系统调用pgaccess，报告已经被访问过的页。系统调用接收3个参数，参数1 希望检查的第一个页所在的虚拟地址；参数2 希望检查的页的数量；参数3 bitmask的地址

# 分析

**bitmask的定义**

数据结构bitmask对每1页使用1bit来确认页的访问状态，第一页对应最低位的bitmask。

**系统调用pgaccess的注册**

调用系统调用pgaccess的程序已经在user/usys.S中注册；系统调用号在kernel/syscall.h中定义；只需要在kernel/sysproc.c中实现sys_pgaccess函数。

**定义PTE_A**

根据xv6book中提供的Figure 3.2: RISC-V address translation details，在kernel/risv.h中定义PTE_A

```c
#define PTE_A (1L << 6)
```

**需要完成的功能**

1. 使用argaddr函数和argint函数读取传入系统调用pgaccess的参数 
2. 遍历需要访问的页的页表项，记录页表项PTE_A到bitmask，访问结束后记得重置PTE_A为0

# 实现

可以参考mappages函数访问给定虚拟地址对应页的写法

```c
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 staddr;//其实虚拟地址
  int nums;//要访问的页的数量
  uint64 useraddr;//bitmask的用户虚拟地址
  if(argaddr(0, &staddr) < 0 || argint(1, &nums) < 0 || argaddr(2,&useraddr))//从trapframe中读取传入系统调用的参数
    return -1;
  uint64 a = PGROUNDDOWN(staddr);//将虚拟地址转换成对应页的起始地址(只保留页号，页内偏移置为0)
  uint64 last = a+nums*PGSIZE;//访问到last结束
  pte_t *pte;
  uint64 bitmask = 0x0000000000000000;//定义bitmask为64位(题目也没说要多少位)
  int cnt = 0;
  for(;;){
    if(a == last)//访问完成所有页则退出循环
      break;
    if((pte=walk(myproc()->pagetable,a,1))==0)//读取对应页表项
        return -1;
    if(*pte & PTE_A){//如果PTE_A位1，则在bitmask的对应位置记录
      bitmask = bitmask | (1L << cnt);
    }
    else{
      ;
    }
    if(copyout(myproc()->pagetable, useraddr, (char*)&bitmask, sizeof(bitmask)) < 0)//复制内核中的bitmask到用户进程地址空间下
      return -1;
    *pte = *pte & (~PTE_A);//重新设置 PTE_A 为 zero
    cnt++;
    a += PGSIZE;//访问下一页
  }
  return 0;
}
```

因为在sys_pgaccess中使用了walk函数获取页表项，需要在kernel/defs.h中补充walk函数的定义

```c
pte_t *         walk(pagetable_t, uint64,int);
```



