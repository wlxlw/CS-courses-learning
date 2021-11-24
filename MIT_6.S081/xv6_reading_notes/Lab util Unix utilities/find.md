**知识点:** 

1. 目录的内容是什么?

   在xv6中目录就像一个文件，只不过存储的内容是一个个的dirent。

    ```c
    struct dirent { //dirent结构体
      ushort inum; //inum为0表示当前的dirent是空闲的
      char name[DIRSIZ]; 
      //该目录下的一个文件(或目录)的名字  DIRSIZ为14
    }; 
    ```

2. 文件的状态信息(stat结构体)

   ```c
   #define T_DIR     1   // Directory
   #define T_FILE    2   // File
   #define T_DEVICE  3   // Device
   struct stat {
     int dev;     // File system's disk device
     uint ino;    // Inode number
     short type;  // Type of file   
     short nlink; // Number of links to file
     uint64 size; // Size of file in bytes
   };
   ```

3. 如何读取文件或者目录的状态信息？(stat函数 fstat函数的用法)

   将文件名/文件描述符和相关的数据结构关联

   ```c
   int stat(const char*path, struct stat*st);
   //读取path所指文件的状态信息到st
   
   int fstat(int fd, struct stat*st);
   //读取fd所指文件的状态信息到st
   ```

**任务要求**：实现find程序(find path filename)，能够找到指定目录path下所有文件名为filename的文件。

**实现：** 

 1. ：阅读ls.c，学习怎么读取目录的内容

 2.  ```
    dfs(path, filename)
      遍历path下每一个(文件/目录)：
        如果是目录：
            dfs(该目录，filename)
        否侧：
            输出当前文件的完整路径 
     ```

**参考**

dirent结构体的相关解释https://pdos.csail.mit.edu/6.828/2012/lec/l-fs.txt