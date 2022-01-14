# Lab 3: The TCP Sender

从远程主机接收确认号ack和窗口大小window_size，并从本地主机的ByteStream中读取数据，组合中segment之后，发送给远程主机

TCP Sender与TCP segment

1. TCP Sender向segment**写入序号**sequence number，SYN，FIN，payload
2. TCP Sender从segment**读取**ackno，window size

TCP Sender需要实现

1. 控制发送窗口大小。处理接收方发送的ackno和window size，即由接收方控制发送方的发送窗口大小
2. 构造segment。从ByteStream中读取数据，添加seqno，SYN，FIN后构造segment
3. 追踪丢失的segment。追踪哪些segment发送了，但是没有被接受方所接收 (这种segment被称 outstanding segment)
4. 超时重传。超过一定时间后如果outstanding segment还没有被接收，则重新发送。

TCP Sender怎么知道报文丢失了？

一个报文段，在**足够长的时间后**，还是没有**全部**被接收(fully acknowledged)，那么这个报文段就要重新发送。

如何定义足够长的时间？

1. 通过调用TCPSender’s tick获得，上一次调用TCPSender’s tick到此次调用TCPSender’s tick之间的时间间隔
2. 在构造TCPSender的时候，需要初始化重新传输时间retransmission timeout (RTO)。RTO会不断变化，但初始值是一样的、
3. 实现一个重新传输计时器retransmission timer，调用timer后，当RTO消耗完，则发出警报
4. 

