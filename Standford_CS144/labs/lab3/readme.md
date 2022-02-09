# Lab 3: The TCP Sender

从远程主机接收确认号ack和窗口大小window_size，并从本地主机的ByteStream中读取数据，组合中segment之后，发送给远程主机

## TCP Sender与TCP segment

1. TCP Sender向segment**写入序号**sequence number，SYN，FIN，payload
2. TCP Sender从segment**读取**ackno，window size

## TCP Sender需要实现

1. 控制发送窗口大小。处理接收方发送的ackno和window size，即由接收方控制发送方的发送窗口大小
2. 构造segment。从ByteStream中读取数据，添加seqno，SYN，FIN后构造segment
3. 追踪丢失的segment。追踪哪些segment发送了，但是没有被接受方所接收 (这种segment被称 outstanding segment)
4. 超时重传。超过一定时间后如果outstanding segment还没有被接收，则重新发送。

## TCP Sender怎么知道报文丢失了？

一个报文段，在**足够长的时间后**，还是没有**全部**被接收(fully acknowledged)，那么这个报文段就要重新发送。

如何定义足够长的时间？

1. 通过调用TCPSender’s tick获得，上一次调用TCPSender’s tick到此次调用TCPSender’s tick之间的时间间隔
2. 在构造TCPSender的时候，需要初始化重新传输时间retransmission timeout (RTO)。RTO会不断变化，但初始值是一样的、
3. 实现一个重新传输计时器retransmission timer，调用timer后，当RTO消耗完，则发出警报
4. 发送一个包含序号的报文(不管是第一次发的还是重发的)后，如果计时器没有工作，则启动计时器
5. 当一个报文段的发送的数据都被接受，则停止重新传输计时器
6. 当tick函数被调用且重新传输计时器超时后
   1. 重新传输**序号最小的**且**没有被完全接收的**报文段。需要保存这些报文段在内部的数据结构中
   2. 当发送窗口不为0时
      1. 追踪需要连续重传的次数，TCPConnection需要这些信息判断连接是否需要终止
      2. double 重新传输时间RTO，进行指数退避(exponential backoff)，避免加剧网络拥塞
   3. 重新启动重新传输计时器
7. 发送方接收到接收方发出的ackno
   1. 设置重新传输时间为初始值initial value
   2. 如果报文中的数据还有一部分没有被接收，则重新启动重新传输计时器
   3. 设置连续重传数量consecutive retransmissions为0

## Implementing the TCP sender

TCP sender的状态变化

1. "CLOSED"关闭状态
2. "SYN_SENT"已经发送了连接请求报文，但完全没有被接受
3. "SYN_ACKED" 连接建立完成，开始传输数据
4. "SYN_ACKED also" 应用程序不再写入数据 但发送的数据没有被完全接收
5. "FIN_SENT" 发送了FIN报文，但没有被接收
6. "FIN_ACKED" FIN报文被接收

---

### TCPsender中的成员变量

```c++
//lab中已经给出的成员变量
WrappingInt32 _isn;//初始seqno
std::queue<TCPSegment> _segment_out //已发送的报文段
unsigned int _initial_retransmission_timeout;//初始重传时间
uint64_t _next_seqno{0};//absolute seqno
ByteStream _stream //发送应用程序写入的，但还没发送的数据

//新定义的成员变量 
uint64_t _bytes_in_flight{0};//已发送未接受的字节数量
uint16_t _window_size{1};//发送传送大小
std::list<TCPSegment> _retrans_queue{};//重传报文队列
std::list<TCPTimer> _retrans_timer_queue{};//重传计时器队列
std::list<int> _retrans_s{};//报文重传次数队列
int _consecutive_retransmissions{0};//最大连续重传次，TCP会根据该值判断是否需要终止连接
bool _zero_window{false};//当前发送窗口是否为0
WrappingInt32 _ackno;//确认号
```



### 重传计时器&如何实现重新传输报文

在(携带序号的)报文段被送入`_segments_out`的同时，

将报文段送入`_retrans_queue`；新建一个重传计时器送入`_retrans_timer_queue`

当TEST样例调用tick的时候，更新`_time = _time+tick_time`

在TCPSender中新增成员变量：

```c++
size_t _time;//记录时间
std::list<TCPSegment> _retrans_queue;//保存发送的报文副本
std::list<TCPTimer> _retrans_timer_queue;//报文对应的重传计时器
std::list<bool> _retrans_s;//保存报文的发送状态，如果报文已经被完全接收，状态更新为true
```

重传计时器类

```
TCPTimer
	私有变量private:
		重新传输时间_RTO
		_time
	开放接口public:
		构造函数:
			初始化_RTO和_timer
		start函数:
			重新开始计时_time=0
		update函数:
			读取传入的时间cpu_time
			更新_time
			_time += cpu_time
		is_retrans函数判断是否需要重新传输:
			if _time>=_RTO:
				返回 true
			else
				返回 false
		double_rto:
			_RTO *= 2 实现指数退避		
```

### fill_window() 创建并发送报文段

```c++
void TCPSender::fill_window()
```

fill_window函数需要用TCP报文填充发送窗口：

TCP报文有三种类型:

1. SYN报文(不包含数据)
2. FIN报文(可以包含数据)
3. DATA报文

在填充发送窗口的时候，需要注意TCP报文的最大长度为1425Bytes，而窗口大小为最大为65535Bytes。所以，fill_window函数每次可能需要创建多个报文来填充发送窗口

#### 如何根据窗口大小判断还能传多少数据？

根据TCPSender记录的变量计算

1. next_seqno函数返回下一个待发送的字节的序号
2. _ackno记录接收方希望接收的下一个自己的序号
3. _window_size记录窗口大小

```
                      _window_size
          |-------------发送窗口--------------------|
|--已接收--|---已发送，未接收---|-----未发送----------------
        _ackno          next_seqno()
        
还能发送的字节数量: _ackno + _window_size - next_seqno()
```

#### 伪代码

```
TCPSegment seg = TCPSegment()
if(还没建立TCP链接连接 则发送SYN报文)
	设置seg的SYN位 seqno位
	发送TCP报文seg
	更新TCP连接发送的字节的absolute seqno
	更新_bytes_in_flight
	**seg送入重传队列**

else if(发送缓存stream_in()结束输入 && 
TCP发送的字节数量等于stream_in()中的所有字节数量+1 &&
发送窗口中还能还未发送的字节数量大于0)
	设置seg的FIN位 seqno位
	发送TCP报文seg
	更新TCP连接发送的字节的absolute seqno
	更新_bytes_in_flight
	**seg送入重传队列**
	
else if(发送窗口中还能还未发送的字节数量大于0 && 
发送缓存stream_in()中还有数据没发送)
	while(发送窗口中还能还未发送的字节数量大于0 
	&& 发送缓存stream_in()中还有数据没发送)
		设置seg的seqno位
		设置seg的paylooad()
			if(还能发送的字节数量大于1452)
				从stream_in()中读取长度为1452到seg.payload()
			else
				读取stream_in()中的所有数据到seg.payload()
		if(满足发送FIN报文的条件)
			设置seg的FIN位
			更新TCP连接发送的字节的absolute seqno
			更新_bytes_in_flight
		发送TCP报文seg
		更新TCP连接发送的字节的absolute seqno
		更新_bytes_in_flight
		**seg送入重传队列**
```

### ack_received()更新确认号`_ackno`和发送窗口`_window_size`

```c++
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size)
```

#### 伪代码

```
if(确认还未发送的数据，确认号非法) ackno>next_seqno()
	return
if(接收方接收到新的内容) ackno>_ackno
	更新_byte_in_flight
遍历重传队列中的每个报文i
	if(当前报文被完全接收,即报文的末尾序号<ackno)
		在重传队列中删除该报文段
	else
		if(接收方接收到新的内容，即接收到的确认号大于保存的确认号)
			初始化报文i的计数器
if(接收到的确认号大于保存的确认号) ackno>_ackno
	更新_ackno=ackno 累计确认
更新窗口大小 _window_size = window_size
if(窗口大小为0) window_size=0
	更新窗口大小为1
	**为了让接收方能发送新的ack，从而在接收方有空余接收空间时重新传输确认号，避免发送方持续处于无法发送的状态**
	更新窗口状态为零窗口 _zero_window=true
else
	更新窗口状态为非零窗口 _zero_window=false
```

### tick()更新重传计时器，并重新传输数据

```c++
void TCPSender::tick(const size_t ms_since_last_tick)
```

#### 伪代码

```
遍历重传队列中的每个报文i，计时器j
	更新计时器j的时间
	对重传队列中的第一个报文
		if(计时器j超时了)
			重新传输
            if(窗口大小！=0) _zero_window==false
                计时器j的RTO翻倍
                报文i的重传次数k++
                if(重传次数k>最大重传次数)
                	更新最大重传次数
            计时器j重新开始计时
```

### send_empty_segment() 发送不包含序号的报文，用于实现发送ACK报文

```c++
void TCPSender::send_empty_segment()
```

#### 伪代码

```
TCPSegment seg = TCPSegment()
seg的序号=next_seqno()
发送seg报文
```

---

### consecutive_retransmissions()返回最大连续重传次数

```c++
unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }
```

### bytes_in_flight() 返回发送了但没被接收的字节的总数

```c++
uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }
```

