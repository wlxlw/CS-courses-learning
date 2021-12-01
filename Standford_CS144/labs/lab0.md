# Lab Checkpoint 0: networking warmup

**Lab0实验指导书** [lab0.pdf (cs144.github.io)](https://cs144.github.io/assignments/lab0.pdf)

**课程说明**

1. 完成实验钱先阅读每个实验的实验指导书
2. 通过Lab一步步搭建自己的router，network interface，TCP protocol。因为每个实验都依赖于前面的实验，所以需要把每一个实验都做得鲁棒性高一点，不然后续的实验无法进行
3. 实验的细节实验指导书中不会完全的给出

**环境配置**

[4种配置环境的途径](https://web.stanford.edu/class/cs144/vm_howto/)

**在VituralBox中配置环境**

[Setting up your CS144 VM using VirtualBox (stanford.edu)](https://web.stanford.edu/class/cs144/vm_howto/vm-howto-image.html)



1. 下载VirtualBox
2. 下载VM image
3. 在VirtualBox导入VM image
4. 在本地使用ssh连接VM

## 实验部分

### Writing webget

#### **任务要求**

利用提供的TCP和socket实现HTTP的GET方法。

##### **知识点**

写代码之前得先读一读 FileDescriptor, Socket, TCPSocket, and Address classes这些类：

[TCPSocket类](https://cs144.github.io/doc/lab0/class_t_c_p_socket.html)可以用来建立两个进程之间的TCP连接。[文档]((https://cs144.github.io/doc/lab0/class_t_c_p_socket.html))中的Detailed Description给出了具体的例子，利用connect方法向其他进程发送TCP连接请求。使用read方法读取从其他进程发送的数据，使用write方法向其他进程发送数据。

[Address类](https://cs144.github.io/doc/lab0/class_address.html) 通过ip地址 端口号 ，或者域名 端口号来标识每个主机中的进程。

再来回顾一下HTTP请求方法的格式：

[HTTP 消息结构](https://www.runoob.com/http/http-messages.html)，注意不能其中的空格以及换行(\\r\\n)。

##### **本地TCP进程如何判断已经读完了服务器发送进程**

阅读 libsponge/util/file descriptor.cc

TCPSocket类有一个eof方法，可以获取eof变量的值，eof变量最开始为0，等到read方法没数据可读时，将eof变量设置为1。所以只要eof为0，就说明还有数据没有读取。

```c++
void FileDescriptor::read(std::string &str, const size_t limit) {
    constexpr size_t BUFFER_SIZE = 1024 * 1024;  // maximum size of a read
    const size_t size_to_read = min(BUFFER_SIZE, limit);
    str.resize(size_to_read);
    ssize_t bytes_read = SystemCall("read", ::read(fd_num(), str.data(), size_to_read));
    if (limit > 0 && bytes_read == 0) {
        _internal_fd->_eof = true;//读完则把eof设置成1
    }
    if (bytes_read > static_cast<ssize_t>(size_to_read)) {
        throw runtime_error("read() read more than requested");
    }
    str.resize(bytes_read);
    register_read();
}
```



##### **思路**

通过Address类来标识我们要访问的提供HTTP服务的服务器进程，再通过TCPSocket向该进程发送连接请求，并写入HTTP的GET请求。

##### 代码

```c++
void get_URL(const string &host, const string &path) {
    // Your code here.
    TCPSocket sock;
    sock.connect(Address(host,"http"));
	//Write HTTP request
    sock.write("GET "+path+" HTTP/1.1\r\n"
                "HOST: " + host + "\r\n"
                "Connection: close\r\n"
                "\r\n");
    // Read HTTP response
    while(sock.eof()==0){
     cout<<sock.read();
    }
    return;
    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).


    cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    cerr << "Warning: get_URL() has not been implemented yet.\n";
}
```



