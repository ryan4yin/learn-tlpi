# 文件 IO: 通用的 IO 模型

## 打开一个文件

系统调用:

```c
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
```

`open` 函数用于打开一个文件，`pathname` 参数指定文件的路径，`flags` 参数指定打开文件的方式。

`flags` 参数的取值如下：

- 文件访问模式:
  - `O_RDONLY`：只读
  - `O_WRONLY`：只写
  - `O_RDWR`：读写
- 文件创建标志:
  - `O_CREAT`：如果文件不存在则创建
  - `O_EXCL`：如果文件存在则报错(可避免在用户代码中检查并创建文件时的并发竞争问题)
  - `O_TRUNC`：如果文件存在则截断
- 文件状态标志:
  - `O_APPEND`：追加写
  - `O_NONBLOCK`：非阻塞
  - `O_SYNC`：同步写
  - `O_DIRECT`：直接 IO, 跳过缓存

## 改变文件偏移量

系统调用:

```c
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

`lseek` 函数用于改变文件的当前偏移量，`fd` 参数是文件描述符，`offset` 参数是偏移量，`whence` 参数指定偏移量的基准。

`whence` 参数的取值如下：
- `SEEK_SET`：从文件开头开始
- `SEEK_CUR`：从当前位置开始
- `SEEK_END`：从文件末尾开始

## 空洞文件

空洞文件是一种特殊的文件，它的文件大小大于实际写入的数据大小。空洞文件的特点是文件中间有一段空白区
域，这段空白区域不占用磁盘空间，但是文件大小会增大。

通过 `lseek` 函数可以将文件的当前偏移量设置到文件末尾之外，这样就可以在文件中间创建空洞。

空洞文件的常见应用场景：

- coredump 文件：当程序崩溃时，操作系统会将程序的内存数据写入到 coredump 文件中。通过空洞文件，可以
  只将程序的实际用到的内存数据写入到 coredump 文件中，使该文件的大小远小于实际内存空间。
- 多线程下载：多线程下载文件时，可以通过空洞文件来实现文件的分段下载，每个线程下载数据的一部分并写入
  到文件的对应位置。
