# 任务要求

实现 StreamReassembler: 将TCP报文段(segment)**按正确的顺序接收**。

SreamReassembler类接收TCP报文段，每个报文段

1. 包含一连串的字节，每个字节都按顺序编号，
2. 并用第一个字节的编号来标识本个报文段。

StreamReassembler类拥有一个ByteStream，StreamReassembler将接收到的TCP报文段按正确的顺序写入ByteStream，使得其他程序可以按顺序从中读取数据。

对StreamReassembler编码过程中可以添加私有成员(private members)，和私有成员函数(private member functions)，但是不能改变公有接口(public interface)

StreamReassembler的接收缓存要存放:

1. 按序到达的，但尚未被应用程序读取的数据
2. 未按序到达的，不能被应用程序读取的数据

# 分析

**capacity包含两个部分：**

1. 按序到达的数据
2. 没有按序到达的数据

因此在StreamReassembler类中，我们需要两种数据结构：

1. 一个ByteSteam对象`_output`，用于存储按需到达的数据
2. 一个**自行设计的数据结构**(作为接收窗口)，用于存储没有按序到达的数据。

**问题0：接收到的数据中哪一部分要送入_output?**

在StreamReassembler类中定义私有成员变量`_ack`，即确认号，用于标记希望接收到的下一个字节的序号，`_ack`初始化为0

**问题1：如何设计一个数据结构用于存储没有按序到达的数据？**

1. 由于这个数据结构和ByteStream对象共享capacity的空间，此**数据结构要能够变长**
2. 由于到达的数据按字节编号，此**数据结构中每一个元素用于存放一个字节**

因此我将接收窗口定义为`deque<string>_win`

**问题2：如何区分接收窗口中哪些部分已经接收到了数据**

定义一个`deque<bool>_win_s`，用来标记_win中哪部分字节已经读取

**问题3：如何控制读取的stream不超过容量capacity**

接收缓存分为两部分，一部分是按序到达的stream，存储在ByteStream中；另一部分是没有按序到达的stream，存储在“接收窗口”中。在定义ByteStream时已经限制了它的容量，**只要确保“接收窗口”的长度合法就行**

```python
在每次接收数据前:
  确认_output和_win的长度等于capacity
```

**问题4：什么时候往ByteStream里写入数据**

每次接收到一个substring时

```python
if substring与接收窗口有交集:#确保准备接收的数据是接收方期望读取的
  将交集写入win
  while 从头遍历win：
    if 遍历到NULL:
      退出循环
    else
      写入数据到_output
```

# 代码

阅读stream_reassembler.hh，声明私有成员变量

```c++
class StreamReassembler {
  private:
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    deque<string>_win; //接收窗口
    deque<bool>_win_s; //标记接收窗口中哪部分已经接收数据
    size_t _ack=0; //确认号
    size_t _win_s_true_cnt;//标记还没有写入_output的字节总数
    size_t _eof_index;//标记要接收的stream中最后的一个字节的序号加一
  public:
    //......
```

阅读stream_reassembler.cc，需要完成

1. 构造函数

   ```c++
   StreamReassembler::StreamReassembler(const size_t capacity):_output(capacity),_capacity(capacity),_win(), _win_s(), _ack(0), _win_s_true_cnt(0),_eof_index(-1) {
   //初始化_ack确认号为0
   //初始化_eof_index为不存在的序号-1
   }
   ```

2. 接收substring的push_substring函数。

   ```c++
   //! \details This function accepts a substring (aka a segment) of bytes,
   //! possibly out-of-order, from the logical stream, and assembles any newly
   //! contiguous substrings and writes them into the output stream in order.
   //data是接收到的substring
   //index是substring第一个字节的序号
   //eof标记当前的data是需要接收的stream中的最后一个substring
   void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
       if(eof){//如果是stream中的最后一个substring
         _eof_index = index+data.size();//标记为stream的末尾序号加一
         if(_ack == _eof_index){//steam已经全部读取完毕
         _output.end_input();
         }
       }
       if(data.size()==0)
         return;
   
       size_t end_index = index+data.size()-1;
       size_t data_start_index;
       size_t stream_start_index;
       size_t stream_end_index;
       // 0         data.size()-1
       //index      end_index
       //     _ack               _ack+_win.size()-1
       if(end_index>=_ack){//data可能在接收窗口内
         stream_start_index = max(_ack,index);//落在接收窗口内的字节流的起始序号
         // stream_end_index = min(_ack+_win.size()-1,end_index);
         stream_end_index = min(_ack+_capacity-_output.buffer_size()-1,end_index);//落在接收窗口内的字节流的末尾序号
         while(_win.size() < _capacity-_output.buffer_size()){//填充接收窗口，保持_win的长度与_output的长度之和等于_capacity
           _win.push_back(" ");
           _win_s.push_back(false);
         }
   
   //printf("%zu %zu\n",_ack+_capacity-_output.buffer_size()-1,end_index);
   //printf("%zu %zu %zu %zu\n",_ack,_capacity,_output.buffer_size(),_output.bytes_read());
   cout<<"**input**string \""<<data<<"\" ack "<<_ack<<" eof "<<eof<<" index "<<index<<endl;
   printf("  stream_start_index:%zu stream_end_index:%zu\n",stream_start_index,stream_end_index);
   //printf("bytestream %zu win %zu\n",_output.buffer_size(),_win.size());
         if(stream_end_index>=stream_start_index){//data存在落在接收窗口内的部分
           data_start_index = stream_start_index-index;
           //data_end_index = stream_end_index-end_index+data.size()-1;
   
           for(size_t i=0;i<=stream_end_index-stream_start_index;i++){//先写入合法的数据到接收窗口内
             if(_win_s[i+stream_start_index-_ack]==false){
             _win[i+stream_start_index-_ack][0] = data[i+data_start_index];
             _win_s[i+stream_start_index-_ack] = true;
             _win_s_true_cnt++;
             }
           }
           while(_win_s.size()>=1 && _win_s[0]){//再从接收窗口的起始位置向_output写入有序数据
   printf("  bytestream write ");
   cout<<"\""<<_win.front()<<"\"";
             _output.write(_win.front());
             _win.pop_front();
             _win_s.pop_front();
             _ack++;
             _win_s_true_cnt--;
   printf(" ack %zu\n",_ack);        
           }
         }
       }
   printf("  ack %zu _eof_index %zu\n", _ack,_eof_index);
       if(_ack == _eof_index){
         _output.end_input();
       }
   }
   ```

3. 返回已经接收到的无序substirngs的数量

   ```c++
   size_t StreamReassembler::unassembled_bytes() const { return _win_s_true_cnt; }
   ```

4. 是否为空

   ```c++
   bool StreamReassembler::empty() const {
     if(_win_s_true_cnt==0 && _output.buffer_empty())//接收窗口没有读取数据，且_output为空
       return true;
     else 
       return false;
   }
   ```

5. 已经实现的访问_output字节流的方法

   ```c++
   const ByteStream &stream_out() const { return _output; }
   ```
