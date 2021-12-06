# 任务要求

实现 StreamReassembler: 将TCP报文段(segment)**按正确的顺序接收**。

SreamReassembler类接收TCP报文段，每个报文段包含一连串的字节，每个字节都按顺序编号，并用第一个字节的编号来标识本个报文段。

StreamReassembler类拥有一个ByteStream，StreamReassembler将接收到的TCP报文段按正确的顺序写入ByteStream，使得其他程序可以按顺序从中读取数据。

对StreamReassembler编码过程中可以添加私有成员(private members)，和私有成员函数(private member functions)，但是不能改变公有接口(public interface)

StreamReassembler的接收缓存要存放:

1. 按序到达的，但尚未被应用程序读取的数据
2. 未按序到达的，不能被应用程序读取的数据