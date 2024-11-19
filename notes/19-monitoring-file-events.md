# 监控文件事件

> https://docs.kernel.org/filesystems/inotify.html

> https://man7.org/linux/man-pages/man7/inotify.7.html

`inotify` 是 Linux 内核提供的一种文件事件监控机制, 用于监控文件系统的变化.

## inotify API

```c
#include <sys/inotify.h>

// 初始化 inotify 实例, 返回一个文件描述符
// 可从该文件描述符读取 inotify 事件
int inotify_init(void);

// 初始化 inotify 实例, 并指定 flags
int inotify_init1(int flags);

// 添加监控目录或文件
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);

// 移除监控目录或文件
int inotify_rm_watch(int fd, int wd);
```

`inotify_add_watch` 函数的 `mask` 参数是一个位掩码, 用于指定监控事件类型, 具体的事件类型请参考 `man 7 inotify`, 如下列出一部分:

- `IN_ACCESS`: 文件被访问
- `IN_MODIFY`: 文件被修改
- `IN_ATTRIB`: 文件元数据被修改
- `IN_CLOSE_WRITE`: 关闭以写方式打开的文件
- `IN_CLOSE_NOWRITE`: 关闭以只读方式打开的文件
- ...


## inotify 队列限制

inotify 事件队列需要消耗内核内存, 因此 Linux 内核对其做了一些限制:

- `/proc/sys/fs/inotify/max_user_instances`: 一个用户可以创建的 inotify 实例数量
- `/proc/sys/fs/inotify/max_user_watches`: 一个用户可以创建的监控数量
- `/proc/sys/fs/inotify/max_queued_events`: 一个 inotify 实例的队列中可以排队的事件数量

这些限制可以通过修改 `/proc/sys/fs/inotify/` 目录下的文件进行动态调整, 也可以通过修改 `/etc/sysctl.conf` 文件进行永久调整.


