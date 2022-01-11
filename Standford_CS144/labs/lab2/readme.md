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

## 捋一捋TCPReceiver、StreamReassembler、ByteStream之间的关系

### 分模块介绍

ByteStream：将接收到的substring按序写入，并按序读出。

StreamReassembler：接收**没有按序**到达的substring，将其按编号重新排序。

TCPReceiver：接收segment，并对接收到的segments按编号重新排序，计算ackno和windows size

### 自顶向下介绍

TCPReceiver接收来自发送方的segments，通过StreamReassembler按编号重新排序segments，并将重新排序之后的数据写入ByteStream

## TCPReceiver

### 构造函数

```cc
class TCPReceiver {
    //! Our data structure for re-assembling bytes.
    StreamReassembler _reassembler;

    //! The maximum number of bytes we'll store.
    size_t _capacity;
    
    // lab 2
    WrappingInt32 _seqno;
    WrappingInt32 _isn;
    WrappingInt32 _ack;
    size_t _abs_seqno;
    size_t _written_cnt;
    bool _syn_flg;
    WrappingInt32 _end_seqno;
  public:
    TCPReceiver(const size_t capacity) : _reassembler(capacity), _capacity(capacity),_seqno(0),_isn(0),_ack(0),_abs_seqno(0),_written_cnt(0),_syn_flg(false),_end_seqno(0){};
    //lab2
//其他代码省略
```

### segment_received方法

#### 处理接收到的TCP segment

**注意segment中的序号和StreamReassembler中使用的序号不同**

#### 需要用到的方法

[TCPSegment类](https://cs144.github.io/doc/lab2/class_t_c_p_segment.html#a41eb3ff25fee485849fd38eb31c990d6)

1. length_in_sequence_space方法：返回segment包含的序号(考虑了SYN和FIN)
2. header方法：返回segment的TCP首部

[TCPHeader类]([Sponge: TCPHeader Struct Reference (cs144.github.io)](https://cs144.github.io/doc/lab2/struct_t_c_p_header.html#details))

1. syn成员变量
2. fin成员变量

#### 伪代码

```
处理序号:
WrappingInt32 seqno
WrappingInt32 isn
WrappingInt32 ack
size_t abs_seqno
size_t written_cnt
bool syn_flg
WrappingInt32 end_seqno

if 是SYN==1的报文段:
	初始化序号:
		abs_seqno=0
		isn = 报文段的seqno
		ack = seqno+1
		syn_flg = true
if 报文段携带数据:
	往_reassembler里送入数据, 提供参数:
		字符串
		字符串首个序号:
            if 报文段SYN不为1
				= unwrap(seqno,isn,abs_seqno)-1
			else if 报文段SYN为1
				= unwrap(seqno+1,isn,abs_seqno)-1
		eof:
			if FIN=1 eof设置为true
			else 设置为false
		written_cnt = 本次成功写入ByteStream的字节数量
		更新序号:
			abs_seqno += written;
			ack = wrap(abs_seqno+1,isn)
if 是FIN==1的报文段
	//设置end_seqno为FIN对应的序号
	end_seqno = seqno + 报文段长度(包括SYN FIN) -1
if end_seqno == ack
    bytestream结束输入
	abs_seqno++;//更新abs_seqno
    ack = wrap(_abs_seqno+1,_isn);//更新ack
```

TEST CASE:

SYN=1也能带数据 syn 1 fin 0 data "Hello, CS144!" (SYN=1的报文也能携带数据有点怪)

SYN和FIN可以同时为1 syn 1 fin 1 seqno 5 data ""

#### 代码

```cc
void TCPReceiver::segment_received(const TCPSegment &seg) {
    //size_t seq_no= seg.length_in_sequence_space();
    cout<<"syn "<<seg.header().syn<<" fin "<<seg.header().fin<<" seqno "<<seg.header().seqno<<" data \""<<seg.payload().copy()<<"\""<<endl;
    WrappingInt32 now_seqno = seg.header().seqno;
    if(seg.header().syn){
        _isn = WrappingInt32(seg.header().seqno);
        _ack = _isn+1;
        _abs_seqno = 0;
        _syn_flg = true;
    }
    if(seg.payload().copy().size()!=0){
        bool eof = false;
        size_t  now_written;
        if(seg.header().fin)
            eof = true;
        if(seg.header().syn){//如果是syn报文，其携带的数据的序号为seqno+1
            now_seqno = now_seqno + 1;
        }
        if(seg.payload().size()!=0){
            now_written = stream_out().bytes_written();
            _reassembler.push_substring(seg.payload().copy(),
            unwrap(now_seqno,_isn,_abs_seqno)-1,
            eof);
            _written_cnt = stream_out().bytes_written() - now_written;
            //update seqno
            _abs_seqno += _written_cnt;
            _ack = wrap(_abs_seqno+1,_isn);
        }
    }
    if(seg.header().fin){
        _end_seqno = seg.header().seqno + seg.length_in_sequence_space()-1;
    }
    if(_end_seqno == _ack){
        stream_out().end_input();
        _abs_seqno++;
        _ack = wrap(_abs_seqno+1,_isn);
    }
}
```

### ackno方法

返回确认号ack

1. if(ISN没有设置，即没有收到设置了SYN的报文)

   ​	返回empty optional

   else

   ​	返回确认号

```cc
optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_syn_flg) 
        return _ack;
    else
        return {};
}
```

### window_size方法

返回接收方的接收窗口的长度

```cc
size_t TCPReceiver::window_size() const { return {_capacity-stream_out().buffer_size()}; }
```