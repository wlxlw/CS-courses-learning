**任务要求**：使用pipe实现进程间通信

1. 父进程向子进程发送"message from parent"；子进程 print "<pid>: received ping"
2. 子进程 向父进程发送"message from child"；子进程exit；父进程接受子进程发送的消息，父进程exit

**提示**：

1. 对空管道read，调用read的进程阻塞，知道数据足够读取；对满管道write，调用write的进程阻塞，直到有足够空间写入 (见 man 7 pipe)。

2. 两种写法，一种用一个管道实现半双工通信，另一个写法使用两个管道实现全双工通信。

3. 使用单管道的写法时，父进程和子进程的 “write 管道” 和“read管道”要配对 (父先write则子先read；父先read则子先write)。

4. 使用双管道的写法时，父子进程 输出"<pid>: received ping"和 "<pid>: received pong"要有一个先后顺序，不能两个进程同时可以从管道读取数据，这样会导致在终端上的输出是乱序的。具体操作一个进程的printf在write之前，另一个在write之后。

   

