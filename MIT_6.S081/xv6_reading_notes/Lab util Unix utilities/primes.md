**任务要求：** 用递归的方法实现筛法求素数。

**算法伪代码：**

```c
//递归
void primes(leftPipe){
    if 不能读取左pipe的数据
        退出
    else 可以读取读取左pipe的数据
        创建右管道rightPipe
        fork
        对于子进程:
         调用primes(rightPipe)
        对于父进程:
         输出从左管道读取的第一个数据
         继续从左管道读取数据，并将数据筛选后写入右管道
         primes(rightPipe)
         等待子线程完成
}
```

