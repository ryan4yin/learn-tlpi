# 文件系统

## Linux 下的主流文件系统

> 2024 年 11 月

1. **ext4 与 xfs**: 目前服务器上主流的文件系统, 两者都是日志文件系统.
2. **btrfs**: 一个新兴的文件系统, 有很多先进的特性, 但是在数据库场景下性能不如老牌的 ext4 和 xfs.
3. zfs: 一个非常先进的文件系统, 未被 Linux 内核支持, 但许多商业公司在其产品中使用了 zfs 或者其衍生版
   本.
4. bcachefs: 一个类似 zfs 的现代文件系统, 在 Linux 6.7 版本中被合并到了主线内核, 目前仍处于实验阶段.

## 虚拟文件系统 VFS

> https://docs.kernel.org/filesystems/index.html

Linux 内核创建了一个虚拟文件系统 VFS, 它是一个文件系统的抽象层, 任何实现了 VFS 接口的文件系统都可以
挂载到 VFS 上, 从而被用户空间程序访问.

VFS 定义了几个通用的数据结构:

1. **Superblock**: 包含一个已挂载的文件系统的元数据, 如文件系统类型, 挂载点, 挂载标志等.
2. **Inode**: 文件系统中的文件(目录也是一个文件), 包含文件的元数据, 如文件大小, 创建时间, 修改时间等.
   - 对于一个文件而言，它的文件名只是一个标签，是可以改变的，但是 inode 是不变的，它对应着一个真正的
     文件。
3. **File**: 文件描述符, 用于表示被一个进程打开的文件.
   - 用于进程与文件之间进行交互的一个数据结构，它是一个纯软件的对象，没有关联的磁盘内容
4. **Directory Entry (dentry)**: 存储目录和文件的链接信息


TODO



## 文件系统的挂载

`/proc/mounts` 文件记录了当前系统上挂载的文件系统信息.

可通过 `mount` `umount` 命令挂载和卸载文件系统, 它们也可以通过系统调用的方式调用.

```c
#include <sys/mount.h>

// 挂载文件系统
int mount(const char *source, const char *target, const char *fstype, unsigned long mountflags, const void *data);
// 卸载文件系统
int umount(const char *target);
// 卸载文件系统，可以指定额外的选项
int umount2(const char *target, int flags);
```

挂载文件系统时, 其 `mountflags` 参数可以是以下值的组合:

> 注: atime - access time, ctime - change time, mtime - modify time.

> https://docs.rs/nix/latest/nix/mount/struct.MsFlags.html

- `MS_BIND`: 绑定挂载. 将一个目录挂载到另一个目录上, 两个目录共享相同的 inode.
- `MS_DIRSYNC`: 目录同步. 强制使目录的更新同步到磁盘.
  - 其效果类似 open 的 `O_SYNC` 选项, 但只对目录有效.
- `MS_MANDLOCK`: 允许强制锁定文件.
- `MS_MOVE`: 移动挂载. 将一个挂载点移动到另一个挂载点.
  - 与先 `umount` 再 `mount` 相比, `MS_MOVE` 更高效, 而且是原子操作.
  - 与 `mount --move` 命令相对应.
- `MS_NOATIME`: 不更新访问时间.
  - 通常用在不关心 atime 的场景下, 可提高性能, 减少磁盘 I/O.
- `MS_NODEV`: 不允许访问此文件系统上的设备文件(块设备或字符设备), 保障文件系统的安全性.
- `MS_NODIRATIME`: 不更新目录访问时间
  - 与 `MS_NOATIME` 类似, 但只对目录有效.
- `MS_NOEXEC`: 不允许将此文件系统上的文件作为可执行文件运行.
- `MS_NOSUID`: 不允许设置 set-user-ID 和 set-group-ID 位. 保障文件系统的安全性.
- `MS_RDONLY`: 只读挂载.
- `MS_RELATIME`: 相对 atime. 只有在 atime 小于等于 mtime 或 ctime 时才更新 atime.
  - 相比 `MS_NOATIME`, `MS_RELATIME` 同样能提高性能, 但又不会完全关闭 atime.
  - 用户能观察到自上次文件被更新以来, 有无被读取过.
- `MS_REMOUNT`: 重新挂载. 用于修改已挂载文件系统的挂载参数.
- `MS_SYNCHRONOUS`: 同步挂载. 所有文件 I/O 操作都会立即同步到磁盘.


另外 `mount2` 还有一个 `flags` 参数, 可以是以下值的组合:

> https://docs.rs/nix/latest/nix/mount/struct.MntFlags.html

- `MNT_FORCE`: 强制卸载文件系统.
- `MNT_DETACH`: 从挂载点分离文件系统, 但不卸载.
- `MNT_EXPIRE`: 将文件系统标记为过期.
  - 第一次调用 `umount2` 时, 文件系统不会被卸载, 并且该调用一定会失败并返回
    - `EAGAIN`: 文件系统空闲, 将标记为过期, 但不会立即卸载.
    - `EBUSY`: 文件系统仍在被使用, 无法卸载.
  - 第二次调用 `umount2` 时, 若文件系统空闲, 将被立即卸载.
  - 其用途是卸载一段时间内未被使用的文件系统.

## 高级挂载特性

1. 一个文件系统可以同时挂载到多个挂载点上.
2. 同一挂载点上可以挂载多个文件系统, 后挂载的文件系统会覆盖先挂载的文件系统.
3. 绑定挂载可以将一个目录挂载到另一个目录上, 两个目录共享相同的 inode.
   - docker 的文件夹挂载就是通过 bind 挂载实现的.
4. 递归绑定挂载: 不但挂载目录本身, 还会挂载目录下的所有子挂载点.
   - 通过 `mount --rbind` 命令实现.

## tmpfs 临时文件系统

tmpfs 是一个基于内存的文件系统, 它的数据存储在内存中, 速度非常快, 但是不会持久化到磁盘.

NixOS 的 tmpfs as root 就利用了 tmpfs 不会持久化的特性, 确保系统中任何未明确持久化的数据都将在
重启后被清空.

## 获得与文件系统相关的信息

```c
#include <sys/statfs.h>

// 获取文件系统信息, Linux 专有
int statfs(const char *path, struct statfs *buf);
// 获取 fd 所在文件系统的信息, Linux 专有
int fstatfs(int fd, struct statfs *buf);

// 获取文件系统信息, 通用
int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);
```

