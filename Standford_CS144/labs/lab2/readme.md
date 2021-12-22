# overview

在lab2中，我们需要实现TCPReceiver，TCPReceiver接收来自网络的segments，接着将接收到的数据送入StreamReassembler，最终数据被送入ByteStream。

TCPReceiver需要告知发送方(sender)

1. **确认号**，即接收方希望收到的第一个字节的编号
2. **窗口大小**，即**从第一个无序到达的字节**与**第一个不能接收字节**下标之间的距离

接收窗口对应的字节序号：[ack, ack+window_size)

本节的难点在于思考TCP如何使用sequence number代表每一个字节在stream中的位置

# Lab2: The TCP Receiver

## 64比特indexes与32比特seqno之间的转换

在StreamReassembler中我们使用64比特的序号来标记substring中的字节序号，且起始序号为0。然而在TCP首部中，空间比较紧张，每一个字节的序号需要用32比特的"seqiemce number"来表示，这会产生三个难点。

1. 我们希望通过TCP传送的字节流可以是任意长度的，然而TCP报文段中的seqnos只有32比特，当sqenos为2^32^ -1时，下一个sqeno应为0。即**采用循环计数**
2. sqenos的**起始序号需要是随机的**。因为这样设计比较安全，防止属于前一TCP连接old segment被新的TCP连接接收。通过Initial Sequence Number(ISN)来获得第一个seqno
3. **TCP逻辑上的开始与结束都要占用一个sqeno**。为了确保TCP的**连接请求报文**和**连接释放报文**能够可靠的被接收，TCP报文中包含SYN与FIN标志位的报文需要额外占用一个序号。

发送"cat"对应不同的序号

|        element |   syn    |    c     |  a   |  t   | Fin  |
| -------------: | :------: | :------: | :--: | :--: | :--: |
|          seqno | 2^32^ -2 | 2^32^ -1 |  0   |  1   |  2   |
| absolute seqno |    0     |    1     |  2   |  3   |  4   |
|   stream index |          |    0     |  1   |  2   |      |

### 实现

1. WrappingInt32 wrap(uint64 t n, WrappingInt32 isn)

   将**absolute seqno** n转换成**seqno** isn。

2.  uint64 t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64 t checkpoint)

   将**seqno** isn转换成**absolute seqno** n。

判题机制是比较标准输出和答案之间的区别，所以代码中不要有任何标准输出语句(cout,printf等)  搞得我代码写完半天不知道错在哪里



```
      if(checkpoint-ans>(MAX32/2) && ans+MAX32>checkpoint){ //这里必须要加ans+MAX32>checkpoint，不然在ans+MAX32溢出时，
                                                            //ans+MAX32的到checkpoint的距离=checkpoint-(ans+MAX32)>ans到checkpoint之间的距离
                                                            //而不=(ans+MAX32)-checkpoint
```

