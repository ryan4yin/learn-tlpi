# 文件属性

## 文件时间戳

Linux 系统不记录文件创建时间, 而只提供了三个时间戳:

- `atime`: 最后一次访问(access)时间
- `mtime`: 最后一次修改(modify)时间
- `ctime`: 最后一次元数据(状态)修改(change)时间

各系统调用对文件时间戳的影响如下:

| 系统调用        | 目标 atime | 目标 mtime | 目标 ctime | 父目录 atime | 父目录 mtime | 父目录 ctime | 备注                                                                  |
| --------------- | ---------- | ---------- | ---------- | ------------ | ------------ | ------------ | --------------------------------------------------------------------- |
| chmod()         |            |            | [x]        |              |              |              | 同样适用于 fchmod()                                                   |
| chown()         |            |            | [x]        |              |              |              | 同样适用于 lchown() 和 fchown()                                       |
| exec()          | [x]        |            |            |              |              |              |                                                                       |
| link()          |            |            | [x]        |              | [x]          | [x]          | 影响第二个参数的父目录                                                |
| mkdir()         | [x]        | [x]        | [x]        |              | [x]          | [x]          |                                                                       |
| mkfifo()        | [x]        | [x]        | [x]        |              | [x]          | [x]          |                                                                       |
| mknod()         | [x]        | [x]        | [x]        |              | [x]          | [x]          |                                                                       |
| mmap()          | [x]        | [x]        | [x]        |              |              |              | 仅在更新 MAP_SHARED 映射时才会改变 st_mtime 和 st_ctime               |
| msync()         |            | [x]        | [x]        |              |              |              | 仅在文件被修改时改变                                                  |
| open(), creat() | [x]        | [x]        | [x]        |              | [x]          | [x]          | 创建新文件时                                                          |
| open(), creat() |            | [x]        | [x]        |              |              |              | 截断现有文件时                                                        |
| pipe()          | [x]        | [x]        | [x]        |              |              |              |                                                                       |
| read()          | [x]        |            |            |              |              |              | 同样适用于 readv(), pread(), 和 preadv()                              |
| readdir()       | [x]        |            |            |              |              |              | readdir() 可能缓存目录项；仅在读目录时更新各时间戳                    |
| removexattr()   |            |            | [x]        |              |              |              | 同样适用于 fremovexattr() 和 lremovexattr()                           |
| rename()        |            |            | [x]        |              | [x]          | [x]          | 同时影响 rename 前后两个父目录的时间戳                                |
| rmdir()         |            |            |            |              | [x]          | [x]          | 同样适用于 remove(directory)                                          |
| sendfile()      | [x]        |            |            |              |              |              | 会改变输入文件的时间戳                                                |
| setxattr()      |            |            | [x]        |              |              |              | 同样适用于 fsetxattr() 和 lsetxattr()                                 |
| symlink()       | [x]        | [x]        | [x]        |              | [x]          | [x]          | 设置链接的时间戳（不是目标文件）                                      |
| truncate()      |            | [x]        | [x]        |              |              |              | 同样适用于 ftruncate()；仅当文件大小改变时才会更新时间戳              |
| unlink()        |            |            | [x]        |              | [x]          | [x]          | 同样适用于 remove(file)；如果之前的链接计数大于 1，文件 st_ctime 改变 |
| utime()         | [x]        | [x]        | [x]        |              |              |              | 同样适用于 utimes(), futimes(), futimens(), lutimes(), 和 utimensat() |
| write()         |            | [x]        | [x]        |              |              |              | 同样适用于 writev(), pwrite(), 和 pwritev()                           |

## 获取文件属性

```c
#include <sys/stat.h>

// 通过文件名获取文件属性, 要求用户对文件父目录有执行(搜索)权限
int stat(const char *pathname, struct stat *buf);
// 通过文件描述符获取文件属性, 用户无需对文件有任何权限
int fstat(int fd, struct stat *buf);
// 通过文件名获取文件属性, 并且不会跟随符号链接(即可获取符号链接本身的属性)
int lstat(const char *pathname, struct stat *buf);
```

## 修改文件时间戳

```c
#include <utime.h>

// 修改文件的 atime 和 mtime 时间戳
// tar/unzip 等工具会使用这个函数来在解压时恢复各文件的 atime 和 mtime 时间戳
int utime(const char *filename, const struct utimbuf *buf);
// 与 utime() 类似, 但是可以指定更精确(微秒级)的时间戳
int utimes(const char *filename, const struct timeval tv[2]);
```

```c
#include <sys/time.h>

// 与 utimes() 类似, 但输入参数是文件描述符
int futimes(int fd, const struct timeval tv[2]);
// 与 utimes() 类似, 但不会跟随符号链接(也就是说可以修改符号链接本身的时间戳)
int lutimes(const char *filename, const struct timeval tv[2]);
```

## 修改文件 owner

```c
#include <unistd.h>

// 通过文件路径修改文件的 owner 和 group
int chown(const char *pathname, uid_t owner, gid_t group);
// 与 chown() 类似, 但是不会跟随符号链接(也就是说可以修改符号链接本身的 owner 和 group)
int lchown(const char *pathname, uid_t owner, gid_t group);
// 与 chown() 类似, 但是通过文件描述符修改文件的 owner 和 group
int fchown(int fd, uid_t owner, gid_t group);
```

## 检查文件权限

```c
#include <unistd.h>

int access(const char *pathname, int mode);
```

其中 `mode` 可以是以下值的组合:

- `R_OK`: 测试读权限
- `W_OK`: 测试写权限
- `X_OK`: 测试执行权限
- `F_OK`: 测试文件是否存在

## 三个特殊权限位

除了常见的 owner, group, others 三种身份的 rwx 权限位外, UNIX 系统还有三个特殊权限位:

- `SUID`: set-user-ID
- `SGID`: set-group-ID
- `sticky`: 粘滞位

前两个之前 09 章已经详细说明过, 这里重点讲一下 sticky 粘滞位.

sticky 粘滞位通常用于目录, 它的作用是: 仅允许文件的 owner, group, root 用户删除文件.

常见用途是 `/tmp` 等多用户共享目录, 若不设置 sticky 粘滞位, 则任何用户都可以删除该目录下的任何文件.

```c
› ls -al /
# ...
drwx------ - root 18 11月 01:45  root
drwxr-xr-x - root 18 11月 02:28  run
drwxr-xr-x - root 30 7月  13:51  srv
dr-xr-xr-x - root 18 11月 23:21  sys
drwxrwxrwt - root 18 11月 23:21  tmp
drwxr-xr-x - root 30 7月  13:51  usr
drwxr-xr-x - root 30 10月 23:34  var
```

这里能看到 `/tmp` 目录的权限位是 `drwxrwxrwt`, 其中最后一个 `t` 就是 sticky 粘滞位.

## 更改文件权限

```c
#include <sys/stat.h>

// 修改文件权限位
int chmod(const char *pathname, mode_t mode);
// 与 chmod() 类似, 但是通过文件描述符修改文件权限位
int fchmod(int fd, mode_t mode);
```

## inode 标志(拓展文件属性)

ext4, Btrfs 等 Linux 文件系统都支持 inode 标志, 用于拓展文件属性.

在 shell 中可以通过 `lsattr`/`chattr` 命令查看与修改文件的 inode 标志.

```sh
› lsattr /etc/passwd
--------------e------- /etc/passwd
```

Linux 支持的 inode 标志如下:

> https://github.com/torvalds/linux/blob/v6.12/include/uapi/linux/fs.h#L244-L306

| `<linux/fs.h>`中的常量 | `chattr`命令参数 | 功能说明                                                                                |
| ---------------------- | ---------------- | --------------------------------------------------------------------------------------- |
| FS_APPEND_FL           | a                | 只能以追加模式打开文件，不能修改或删除已有内容。适用于日志文件。                        |
| FS_COMPR_FL            | c                | 将文件以压缩格式存储在磁盘上。仅 Btrfs/Bcachefs 支持此标志                              |
| FS_DIRSYNC_FL          | D                | 将目录更改同步到磁盘。此标志提供的语义与`mount`的`MS_DIRSYNC`选项等效，但基于每个目录。 |
| FS_IMMUTABLE_FL        | i                | 文件是不可变的：不允许更改文件内容或元数据（权限、时间戳、所有权、链接数等）            |
| FS_JOURNAL_DATA_FL     | j                | 启用文件数据日志记录(需要特权)。                                                        |
| FS_NOATIME_FL          | A                | 访问文件时不更新文件的最后访问时间，可以提供 I/O 性能收益。                             |
| FS_NOCOW_FL            | C                | 文件不会受到写时复制更新影响。此标志仅在支持写时复制语义的文件系统（如Btrfs）上有效。   |
| FS_NODUMP_FL           | d                | 不包括此文件在使用 dump 命令制作的备份中(安全特性)。                                    |
| FS_NOTAIL_FL           | t                | 不使用尾部合并（tail merging）。此标志用于某些文件系统，如ext4。                        |
| FS_SYNC_FL             | S                | 即时更新文件或目录。所有对文件的更改都会立即写入磁盘。                                  |
| FS_UNRM_FL             | u                | 防止意外删除。删除文件时，文件内容会被保存，便于恢复。                                  |
| ...                    | ...              | ...                                                                                     |


举例来说, 我们可以通过 `chattr +i` 命令将文件设置为不可变的, 这样即使 root 用户也无法删除或修改文件.
在 Nix Home Manager 中可通过这种方式保护由它自动管理的文件.



