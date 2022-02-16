# Overview of Past labs

## tcp_receiver 接收并处理TCP报文。

segment->SYN FIN seqno payload-> tcp_receiver->ackno window_size

核心数据结构&函数 :

1. `StreamReassembler _reassembler`

   1. `push_substring(const string &data, const size_t index, const bool eof)`**按照字节序号**对传入到的字节**选择性的接收并重新排序**
      1. **排好序**的字节存放在`ByteStream _output`
      2. **没有排好序但落在接收窗口内**的字节放在`deque<string>_win`

2. `segment_received(const TCPSegment &seg)`

   根据报文首部的不同，**选择性的接收**报文携带的payload到`_reassembler`

   **关注**报文中的**SYN FIN seqno payload**

3. 读取报文之后，更新**确认号ackno**与**窗口大小window_size**

   ```c++
   optional<WrappingInt32> TCPReceiver::ackno() 
   size_t TCPReceiver::window_size()
   ```

4. wrap & unwrap

   将64位的stream index和32位的seqno互相转换

## tcp_sender 发送报文

核心数据结构&函数 :

1. `fill_window()`

   根据当前**TCP连接状态**的**不同**，将发送缓存的数据组合成TCP报文，并发送

2. `ack_received(const WrappingInt32 ackno, const uint16_t window_size) `

   根据接收到的**ackno window_size**更新tcp_sender中保存的**ackno window_size**，并在重传队列中删除传输结束的报文

3. `tick(const size_t ms_since_last_tick)`

   在重传队列中

   1. **更新重传计时器，进行指数退避**
   2. **进行超时重传**
   3. **记录最大重传次数**

4. TCPTime重传计时器类

   1. `bool TCPTimer::is_retrans(size_t RTO)`判断是否超时
   2. `void TCPTimer::double_rto()`超时时进行指数退避

5. `send_empty_segment()`

   发送不包含序号的TCP报文

# Lab 4 : The TCP connection

## 什么时候终止TCP连接(为什么要有time_wait状态)

**关键再与TCP不可靠传输ACK报文，导致TCP连接释放的第四次挥手不一定被对方接收**

### unclean shoutdown

1. TCPConnection某一方发送RST报文
2. outbound和inbound被设置成error状态
3. active()返回false

### clean shutdown

在这种情况下，TCPConnection两端的数据都完全传输完毕。

local TCPConnection使用clean shutdown结束TCP连接的4个前提

1. inbound中的数据完全按序被接收，并且不再接收数据
2. outbound不再写入数据，且已经写入的数据完全发送出去
3. outbound发送的报文被remote完全确认
4. local TCPConnection确定(is confident) romote可以满足3，有a、b两种情况
   1. 当满足前3个条件时，可以确定remote**似乎**(其实不能，因为TCP不确保ack可靠传输)已经接收到local发送的**ack报文**(因为local没有重新传输报文)。**因此，当local满足前3个条件时时，local必须在上一次接收报文之后再等待10倍的初始重传时间，如果在这段时间内没有接收到remote的重传报文，则说明remote已经完全接收了local的发送的ack，local可以启动clean shutdown**
   2. 当满足前3个条件时，且remote是第一个决定释放TCP连接(发起第一次挥手)，此时百分百确认remote满足3

### 终止TCP连接的具体实现

成员变量`_linger_after_streams_finish`用来表示是否需要在两个stream都结束后在保留一段时间TCP连接。

```
if(local不发送数据，但是还接收数据时)
	_linger_after_streams_finish=false
	//local发送TCP连接释放的第一次挥手
if(满足前3个条件)
	if(not _linger_after_streams_finish or 保持TCP连接超过10 * cfg.rt timeout)
        //(A or B)
        //满足A 说明local可以直接关闭 (local第一次挥手
        //满足B 说明local还需要等待一段时间才能关闭 (local第二次挥手
		关闭TCP连接
```

## CODE

### `size_t TCPConnection::write(const string &data)`

三次握手中的第一次在此，四次挥手中主动发送的FIN报文在此

```
size_t TCPConnection::write(const string &data) {
   将data写入_sender的bytestream中
   _sender调用fill_window()发送报文
   TCPConnect发送报文
}
```

### `void TCPConnection::segment_received(const TCPSegment &seg) {`

有限状态机的各种变化处理在此，在不同的状态下，收到的报文的类型是不同的

```
if TCP连接终止
	return
更新time_since_last_segment_received为0
if(rst)//是rst报文
	关闭_sender和_receiver的bytestream
	TCP连接状态设置为终止
else if(listen状态)
	if(收到的是syn报文-TCP连接第一次握手)
		_receiver接收报文
		connect() 主动发送syn ack报文(TCP连接第二次握手)
else if(当前是syn sent状态)
	if(syn && ack -TCP连接第二次握手)
		_receive接收报文
		_sender接收seqno和window_size
		_sender调用send_empty_segment()发送ACK报文
		TCPConnect发送报文
	else if(syn && !ack -遇到双方同时发起TCP连接的特殊状况)
		_receive接收报文
		_sender调用send_empty_segment()发送ACK报文
		TCPConnect发送报文
else if(当前是syn received状态)
	if(ack)
		_receiver接收报文
		_sender接收seqno和window_size
else if(当前是established状态)
	if(fin)
		_receiver接收报文
        _sender接收seqno和window_size
        _sender调用send_empty_segment()发送ACK报文
        TCPConnect发送报文
	else	
        _receiver接收报文
        _sender接收seqno和window_size
        if 接收到的报文不占用序号
            _sender调用send_empty_segment()发送ACK报文
        _sender发送报文
        TCPConnect发送报文
else if(当前是fin wait1状态)
	if(fin -收到对方发送的fin)
		_receiver接收报文
		_sender接收seqno和window_size
		_sender调用send_empty_segment()发送ACK报文
		TCPConnect发送报文
	else if(ack-收到对方对我方发出的FIN的确认)
		_receiver接收报文
		_sender接收seqno和window_size
else if(当前是fin wait2状态)	
		_receiver接收报文
		_sender接收seqno和window_size
		_sender调用send_empty_segment()发送ACK报文
		TCPConnect发送报文
else if(当前是time wait状态)
	if(fin-收到fin报文)//因为remote可能没收到第四次挥手，会再次发送第三次挥手
		_receiver接收报文
		_sender接收seqno和window_size
		_sender调用send_empty_segment()发送ACK报文
		TCPConnect发送报文
else close wait
    _receiver接收报文
    _sender接收seqno和window_size
    _sender发送报文
    TCPConnect发送报文
```

### `void TCPConnection::tick(const size_t ms_since_last_tick)`

```
if(TCP连接终止)
	return
_sender调用tick更新时间
更新上一次接收报文的时间
if(连续重传次数大于指定数值)
	unclean shutdown
	return
调用send_segment发送报文(因为可能有部分报文要重传
```

### `void TCPConnection::send_segment()`

```
while(_sender的发送队列非空){
    segment = _sender发送队列队首
    if(_receiver已经接收过对方发出的数据，即_receiver.ackno().has_value())
    	seg报文的ack设置为true
    	根据_receiver设置seg报文的ackno和win
    _sender发送队列出队一个元素
    发送seg报文
}
clean shutdown
```