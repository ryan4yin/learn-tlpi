# 文件 IO: 深入探究

## 原子操作与竞争条件

一个典型的竞争条件场景是: 两个进程同时尝试检查文件是否存在，如果文件不存在则创建文件。

由于文件的创建操作不是原子操作,而是两个步骤: 检查文件是否存在(尝试以只读方式打开文件)、创建文件
(O_CREAT)，所lf以两个进程可能同时检查到文件不存在并同时尝试创建文件，这样其中一个程序就会lf创建失
败。

比较正确的解法是在打开文件时将 flags 设为 `O_CREAT | O_EXCL`，这样如果文件已经存在则打开失败。

第二个例子则是两个进程同时尝试往文件追加写入数据，也可能导致互相覆盖的情况。因为写入操作也不是原子操
作，而是两个步骤: 移动文件偏移量到文件末尾(lseek)、写入数据(write)。

正确的解法是使用 `O_APPEND` 标志打开文件，这样写入操作会自动将文件偏移量移动到文件末尾,并且避免互相
覆盖.

另一个解法则是使用 `pwrite` 系统调用, 该调用可以在写入数据时指定文件偏移量为 `SEEK_END`, 这样就是一
个原子操作且不会互相覆盖了.

## 文件控制操作 fnctl

`fnctl` 系统调用可用于对一个打开的文件描述符进行各种控制操作。

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */);
```

### 查改文件状态标志

```c
#include <fcntl.h>

// 查询文件状态标志
int flags = fcntl(fd, F_GETFL);
if (flags == -1) {
    perror("fcntl");
    exit(EXIT_FAILURE);
}

// 检查是否设置了 O_APPEND 标志
// 若没有设置则设置
if (flags & O_APPEND) {
    printf("O_APPEND is set\n");
} else {
    printf("O_APPEND is not set, set it now\n");
    flags |= O_APPEND;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}
```

## 文件描述符

文件描述符分为三类:

1. 进程级的文件描述符
   - 针对每个进程, 内核为其维护打开文件的描述符表, 该表的每一条都包含了:
     - 一组文件描述符的控制标志(目前只有一个 close-on-exec 标志)
     - 一个对[打开文件句柄(open file handle)]的引用.
2. 系统级的打开文件表
   - 内核对所有打开的文件维护有一个系统级的打开文件表, 该表的每一条为一个[打开文件句柄], 该句柄存储
     了与一个打开文件相关的全部信息:
     - 文件偏移量
     - 文件状态标志
     - 文件访问模式
     - 对该文件 i-node 对象的引用(所以才会出现删除文件不会立即释放磁盘空间的情况)
     - 其他信息
3. 文件系统的 i-node 表
   - 每个文件系统都会为驻留其上的所有文件创建一个 i-node 表, 每个文件的 i-node 包含如下信息:
     - 文件类型
     - 一个指向该文件所持有的锁的列表的指针
     - 文件的其他各种属性

需要注意的有这几点:

- 多个文件描述符可能指向同一个打开文件句柄, 这样它们就共享了文件偏移量, 文件状态标志, 文件访问模式等
  信息.
- fnctl 调用能修改的是打开文件句柄中的文件状态标志, 因此它它对所有指向该打开文件句柄的文件描述符都生
  效.
- 文件描述符的 close-on-exec 标志是进程级的, 对该标志的修改不会影响任何其他的文件描述符.

## 复制文件描述符

复制文件描述符可以达到多个文件描述符指向同一个打开文件句柄的目的, 这有如下好处:

1. 这些文件描述符共享了文件偏移量, 因此即使未使用 `O_APPEND` 标志, 在追加写入文件时也不会互相覆盖.

相关系统调用:

```c
#include <unistd.h>

// 创建一个 oldfd 的副本, 返回新的文件描述符, 新文件描述符的 id 由系统自动分配
int dup(int oldfd);
// 跟 dup 类似, 优点是可以手动设定新文件描述符的 id
int dup2(int oldfd, int newfd);
// 跟 dup2 类似, 但可以设置新文件描述符的控制标志
// 因为文件描述符目前只有一个 close-on-exec 标志, 所以 flags 参数也只能用于设置该标志
int dup3(int oldfd, int newfd, int flags);
```


## `pread` 与 `pwrite`

`pread` 与 `pwrite` 是 `read` 与 `write` 的变种, 可以指定文件偏移量.


## 向量化 I/O

与编程一样, I/O 操作也可以向量化, 即一次性将多个缓冲区的数据读写到文件中, 或者从文件中读取到多个缓冲区.
其特点是:

1. 操作具备原子性, 不会出现数据竞争.
2. 在数据量较大的情况下, 可以明显提高 I/O 性能.

其缺点则是 API 相比普通系统调用而言稍显复杂.

```c
#include <sys/uio.h>

// 向量化读写
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

// 带偏移量的向量化读写
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
```

## 截断文件

```c
#include <unistd.h>

// 截断文件到指定长度, 若文件原长度小于指定长度则在文件尾部添加一系列空字节或者文件空洞
int truncate(const char *path, off_t length);
// 与 truncate 类似, 但是需要传入文件描述符
int ftruncate(int fd, off_t length);
```

## 创建临时文件

```c
// https://linux.die.net/man/3/mkstemp

#include <stdlib.h>

// 创建一个临时文件, 返回文件描述符, 文件将在文件描述符被关闭时自动删除
// template 参数是一个字符串, 其最后 6 个字符必须是 "XXXXXX", 它们将会被替换为一个唯一的随机字符串
int mkstemp(char *template);
// 与 mkstemp 类似, 但是可以指定文件的 open flags
int mkostemp(char *template, int flags);
```

```c
#include <stdio.h>

// 创建一个临时文件, 返回一个文件流指针供 stdio 库的其他函数使用, 文件将在该流被关闭时自动删除
// tmpfile 会在文件被创建后立即调用 unlink() 删除该文件名, 因此在文件系统中看不到该文件
FILE *tmpfile(void);
```
