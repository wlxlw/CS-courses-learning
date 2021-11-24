**知识点**

1. exec系统调用与char\*argv[]

   ```c
   int exec(char*file,char*argv[])
   //加载file，并以argv作为参数执行
   ```

   argv数组中存放的是一个个指向字符串的指针

2. char\* a与char a[]的区别

   char a[n]: 编译器会给数组a分配n个sizeof(char)的空间。

   char \* a:  编译器会给数组a分配n个sizeof(char\*)的空间。

**任务要求**

实现xargs程序，从标准输入读取额外的输入X，对每一个输入(以\n隔开)执行命令。注意标准输入中最后有一个我们自己从键盘输入(回车)的\n。

**伪代码**

```
while 从标准输入读取数据
	如果读到回车
		fork
		如果是子进程
			读到的数据作为一个新的参数，调用exec执行命令
		否则
			父进程等待子进程执行结束
```

**参考**

[深入 char * ,char ** ,char a[ \] ,char *a[] 内核_行人事，知天命-CSDN博客](https://blog.csdn.net/daiyutage/article/details/8604720)

[MIT 6.S081 Lecture Notes | Xiao Fan (樊潇) (fanxiao.tech)](https://fanxiao.tech/posts/MIT-6S081-notes/)