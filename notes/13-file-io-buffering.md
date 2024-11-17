# 文件 I/O 缓存

出于性能考虑, 系统 I/O 调用与用户空间的库实现都会在进行文件 I/O 时使用缓冲区, 并不会立即读写.

缓冲区分类两层:

- 用户空间的缓冲区
  - 由 C 语言或其他语言的标准库实现, 位于用户空间.
- 内核空间缓冲区
  - 因为有它的存在, 即使用户空间的程序将缓冲区数据全部刷入了内核, 也不代表数据已经落入硬盘.
  - 一个常见的操作是, 在拔出 USB 移动硬盘前, 都建议手动调用一次 `sync` 命令, 确保内核缓冲区的数据都已经写入磁盘.

## stdio 库的缓冲

一个常见的操作是手动调用 `fflush()` 函数强制将 stdio 输出流中的数据刷新到内核缓冲区中.

## 内核缓冲

UNIX 提供了多种缓冲同步模式:

- syncronized I/O data integrity completion
  - 只确保必要的数据已经完成了同步
- syncronized I/O file integrity completion
  - 比 data intergrity 更严格, 它要求所有发生更新的文件元数据都已经同步到了硬盘上. 相对的性能更差.

相关的系统调用如下:

```c
#include <unistd.h>

// 强制使文件处于 Syncronized I/O file integrity completion 状态
int fsync(int fd);

// 强制使文件处于 Syncronized I/O data integrity completion 状态
int fdatasync(int fd);

// 与命令 sync 相同，将所有包含更新文件信息的内核缓冲区数据写入磁盘.
// 在 Linux 中 sync 系统调用会等待所有缓冲区数据写入磁盘后才返回.
void sync(void);
```

相关的 open 系统调用的 flags:

- O_SYNC
  - 打开文件时, 会强制使文件处于 Syncronized I/O file integrity completion 状态.
- O_DSYNC
  - 打开文件时, 会强制使文件处于 Syncronized I/O data integrity completion 状态.
- O_RSYNC
  - 它需要与 O_SYNC 或 O_DSYNC 一起使用才有意义, 它会使得 read 操作也处于 O_SYNC 或 O_DSYNC 所指定的同步状态.


## 就 I/O 模式向内核提出建议

`posix_fadvise` 函数可以向内核提出 I/O 模式的建议, 以便内核更好的优化 I/O 操作.

```c
#include <fcntl.h>

int posix_fadvise(int fd, off_t offset, off_t len, int advice);
```

`advice` 参数的取值:

- POSIX_FADV_NORMAL
  - 默认的 I/O 模式
- POSIX_FADV_RANDOM
  - 进程会随机读取这部分数据, 操作系统可以不用预读取这部分数据, 因为这没啥用.
- POSIX_FADV_SEQUENTIAL
  - 进程会顺序读取这部分数据, 操作系统可以预先读取这部分数据提升性能
- POSIX_FADV_WILLNEED
  - 进程未来会读取这部分数据, 操作系统可以预先将这部分数据读到缓存中
- POSIX_FADV_DONTNEED
  - 未来一段时间内不会读取这部分数据, 操作系统可以释放这部分数据的缓存
- POSIX_FADV_NOREUSE
  - 不会重复读这部分数据, 所以在读取时操作系统可以不用缓存这部分数据


## 绕过内核缓冲区: 直接 I/O

相对 `O_SYNC` 更加严格的同步模式是 `O_DIRECT`, 它会绕过内核缓冲区, 直接将数据写入磁盘.

当然这也是性能最差的同步模式.

另外还需要注意的是, 

- `O_DIRECT` 打开的文件必须是块设备文件, 且文件大小必须是块大小的整数倍.
- 应避免同时以 `O_DIRECT` 与其他带缓冲的同步模式一起打开某一文件, 这会导致数据一致性问题.
