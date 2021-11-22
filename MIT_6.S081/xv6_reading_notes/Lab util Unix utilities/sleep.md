sleep系统调用的定义为int sleep(int)           ( /user/user.h  中记录了系统调用的定义)

**任务要求**: 实现一个用户程序user/sleep.c 实现sleep功能(调用系统调用sleep)

1. 如果用户忘记输入一个参数，返回error
2. 把从命令行读到的字符串参数转换成数字(atoi)
3. 使用系统调用sleep
4. main函数调用exit()来结束程序

**测试user/sleep.c **：

1. 修改Makefile
```bash
UPROGS=\
        $U/_sleep\  #在Makefile文件中添加这一行，让xv6能够找到sleep.c程序
```
3. ./grade-lab-util sleep

