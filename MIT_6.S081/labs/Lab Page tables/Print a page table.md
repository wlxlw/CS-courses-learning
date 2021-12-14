# 任务要求

实现vmprint函数，输出页表的内容。

# 分析

vmprint函数传入根页表的指针pagetable_t作为参数，按规定的格式输出页表的内容。首先输出page table的地址，然后依次输出多级页表(只包含有效的页表项)。

输出的每一行页表项格式为：

多级页表层数 页号 | 页表项内容 |  物理地址

```
page table 0x0000000087f6e000
 ..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
 .. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
 .. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
 .. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
 .. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
 ..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
 .. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
 .. .. ..509: pte 0x0000000021fdd813 pa 0x0000000087f76000
 .. .. ..510: pte 0x0000000021fddc07 pa 0x0000000087f77000
 .. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
```

**阅读freewalk函数代码，参考freewalk函数访问页表的方式**

```c
// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);//递归访问多级页表
      pagetable[i] = 0;//修改bottom-level的页表项
    } else if(pte & PTE_V){//访问到叶子节点找到要访问的物理地址
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}
```

通过递归的方法遍历多级页表树，这里值得注意的时PTE_R这个标记位，PTE_R为1则表示访问到多级页表的叶子节点，即物理地址，PTE_R为0则表示还要继续遍历多级页表，无法直接读取物理地址。

# 实现

再vm.c中补充vmprint函数和vmprint_walk函数

```c
void vmprint_walk(pagetable_t pagetable,int d){
 for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    //if(pte & PTE_V)
    //  printf("*%d*\n",d);
    if((pte & PTE_V) && (pte & (PTE_R)) == 0){//页表项有效，且没有访问到多级页表树的叶子节点
      uint64 child = PTE2PA(pte);
      if(d==1)
        printf(".. ..");
      else
        printf("..");
      printf("%d: pte %p pa %p\n",i,pte,child);
      vmprint_walk((pagetable_t)child,d+1);
    }else if(pte & PTE_V){//访问到多级页表树的叶子节点，获得了物理地址
      printf(".. .. ..%d: pte %p pa %p\n",i,pte,PTE2PA(pte));
    }
  }
}
void vmprint(pagetable_t pagetable){
  printf("page table %p\n",pagetable);
  vmprint_walk(pagetable,0);
}
```

再def.h中补充

```c
void            vmprint_walk(pagetable_t,int);
void            vmprint(pagetable_t);
```