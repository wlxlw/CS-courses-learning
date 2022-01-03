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

   将**absolute seqno** n转换成**seqno** isn

   ```c++
   //! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
   //! \param n The input absolute 64-bit sequence number
   //! \param isn The initial sequence number
   WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
       uint64_t MAX32 = uint64_t(1UL<<32); 
       uint32_t ans_seqno = isn.raw_value();
       n%=MAX32;
       n = uint32_t(n);
       ans_seqno = isn.raw_value()+n;
       return WrappingInt32(ans_seqno);
     
   }
   ```

2. uint64 t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64 t checkpoint)

      将**seqno** isn转换成**absolute seqno** n

      ```c++
      uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
          uint64_t MAX32 = uint64_t(1ul<<32); //MAX32等于2的32次方
          uint64_t ans = n.raw_value()-isn.raw_value();//字节流首尾32位seqno的间距
          if(checkpoint>ans){
            uint64_t num = (checkpoint-ans)/MAX32;
            ans = ans + num * MAX32;
            if(checkpoint-ans>(MAX32/2) && ans+MAX32>checkpoint){//处理上溢出
              ans = ans + MAX32;
            } 
          }   
          else{
            uint64_t num = (ans-checkpoint)/MAX32;
            ans = ans - MAX32*num;
            if(ans-checkpoint>(MAX32/2)&&ans-MAX32<checkpoint){//处理下溢出
              ans = ans - MAX32;
            }
          }
          return ans;
      }
      ```


### **判题机制是比较标准输出和答案之间的区别，所以代码中不要有任何标准输出语句(cout,printf等) **

### **给出的test cases似乎没有考虑编号溢出的情况？**

#### **test case 1：**测试unwrap函数计算与checkpoint距离最近的点时，**上溢出**的问题(lab中没有给出此类测试点)

```c++
uint64 t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64 t checkpoint)
//TEST CASE 1
WrappingInt32 n(0xffffffff);
WrappingInt32 isn(0xf0000001);
uint64_t checkpoint = 0xffffffffffffffff;

//在计算与checkpoint最近的点时，需要从
//序号A(0xffffffff0ffffffe)与序号B(0xf0000001)中选择
//序号A与checkpoint距离的计算方式是 距离=checkpoint-待定点A
//序号B与checkpoint距离的计算方式是 距离=待定点-checkpointB
//距离计算方式的错误会导致错误的结果
```

#### **test case 2：**测试unwrap函数计算与checkpoint距离最近的点时，**下溢出**的问题(lab中给出了此类测试点)

```c++
uint64 t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64 t checkpoint)
//TEST CASE 2
WrappingInt32 n(0xffffffff-1);
WrappingInt32 isn(0x0);
uint64_t checkpoint = 0x0;

//在计算与checkpoint最近的点时，需要从
//序号A(0xfffffffe)与序号B(0xfffffffffffffffe)中选择
//序号A与checkpoint距离的计算方式是 距离d1=待定点A-checkpoint
//序号B与checkpoint距离的计算方式是 距离d2=待定点B-checkpointB
//虽然距离计算方式没有变化，如果代码通过以下方式从A B中选择，会出错：
//    if(距离d1大于2^31)
//	     选点B
```