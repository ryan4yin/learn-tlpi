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
