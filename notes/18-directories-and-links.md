# 目录与链接

## 目录与 inode

- 目录: 一个类似特殊的文件, 其与普通文件的区别在于:
  - 目录的 inode 中记录的文件类型是 `DT_DIR`.
  - 目录的数据块中存储的是目录项(一个文件名及对应 inode 编号的表格), 而不是文件内容.
- inode 表的编号从 1 开始, 1 中记录了文件系统的坏块, 2 则总是文件系统的根目录.

## 硬链接

i-node 中并未记录文件名, 文件名实际上是目录项中的内容, 因此可以有多个文件名指向同一个 inode, 这就是
硬链接, 也可直接称为链接.

- **文件名即为(硬)链接**, 一个 inode 可以有多个文件名(链接), 并且所有文件名(链接)地位平等.
- i-node 中记录了硬链接的数量, 即链接计数(link count), 仅当链接计数为 0 时, 文件系统才会回收 inode.

(硬)链接的限制:

1. i-node 编号是与文件系统绑定的, 因此所有硬链接必须与对应的 inode 在同一文件系统内.
2. 不能为目录创建硬链接, 因为这会导致循环引用.
   - 如果你需要在不同目录下有相同的文件, 可以使用符号链接获得 bind mount.

使用 `ln` 时不带任何参数, 默认会创建硬链接.

如下命令为 `source` 创建了一个硬链接 `target`:

```bash
$ ln source target
```

### FAQ - 如何通过文件描述符反查文件名?

在 05 章中介绍过, 文件描述符包含了内核中打开文件句柄(open file handle)的索引, 而打开文件句柄中只包含
了 i-node 的引用以及文件偏移量等信息, 并不包含文件名.

因此, 从虚拟文件系统 VFS 的角度来看, 从文件描述符无法反查到文件名.

在 Linux 中, 可以通过 `lsof`/`fuser` 命令或者 `/proc/PID/fd` 目录来查看文件描述符对应的文件名:

```bash
# 列出进程 1 的所有文件描述符及其对应的文件路径
$ ls -l /proc/1/fd

# 查看进程 1 打开的文件
lsof -p 1
```

## 符号链接 Symbolic Link (软链接)

软链接比硬链接更加灵活, 也更加常见. 软链接是一个特殊的文件, 其内容是指向另一个文件的路径名.

- 软链接的 inode 中记录的文件类型是 `DT_LNK`.
- 软链接的数据块中存储的是指向另一个文件的路径名.
- 软链接的路径名可以是绝对路径, 也可以是相对路径. 相对路径的参照点是软链接文件本身所在的目录.

软链接的限制:

- 软链接可以指向不存在的文件. 这类链接会在被访问时报错 File not found.
- 软链接可能形成循环引用, 例如 A -> B, B -> A.

软链接的常见用途:

- 软链接可以跨文件系统甚至跨主机, 因此可以用于:
  - 创建跨文件系统的链接.
  - 文件共享, 譬如可以在 Git 仓库中存储软链接.

可以通过 `ln` 命令的 `-s` 选项创建软链接:

```bash
$ ln -s source target
```

### 系统调用对软链接的解释

许多系统调用都会自动解析软链接, 还有一些系统调用则会将软链接视为普通文件处理, 使用时需要注意这一点.

具体的行为请查阅相关系统调用的手册页.

## 链接相关的系统调用

```c
#include <unistd.h>

// 创建硬链接(不会跟随符号链接)
int link(const char *oldpath, const char *newpath);
// 与 link() 类似, 但可以控制是否跟随符号链接
int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
// 删除硬链接(删除一个文件名), 如果删除后 i-node 引用计数为 0, 还会立即回收 inode
// NOTE:: 不能删除目录, 这需要改用 rmdir()
int unlink(const char *pathname);


// 创建软链接
int symlink(const char *oldpath, const char *newpath);
// 对软链接进行解引用
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
```

创建与移除目录有专用的系统调用:

```c
#include <sys/stat.h>

// 创建目录
// NOTE: 不支持递归创建, 即如果父目录不存在, 会返回错误
int mkdir(const char *pathname, mode_t mode);
// 移除目录, 要求目录为必须空
// NOTE: pathname 不能为符号链接
int rmdir(const char *pathname);
```

## stdio 库函数

```c
#include <stdio.h>

// 修改文件名
int rename(const char *oldpath, const char *newpath);

// 移除一个文件或空目录
// NOTE: 在 Linux 上, 此函数会根据文件类型选择调用 unlink() 或 rmdir()
int remove(const char *pathname);

// 解析路径名
// NOTE: realpath() 会解析路径中的所有的符号链接与相对路径, 返回一个绝对路径
char *realpath(const char *path, char *resolved_path);
```

## 访问目录

```c
#include <dirent.h>

// 打开目录
DIR *opendir(const char *name);
// 通过文件描述符打开目录
DIR *fdopendir(int fd);

// 读取目录项
// 每次调用 readdir() 都会返回目录中的下一个文件
struct dirent *readdir(DIR *dirp);

// 重置目录流到目录的开始
void rewinddir(DIR *dirp);

// 关闭目录流
int closedir(DIR *dirp);

// 获取目录流的文件描述符
// 常见用途举例: 将拿到的 fd 传给 fchdir() 实现目录切换
int dirfd(DIR *dirp);
```

## 文件树遍历

```c
#include <ftw.h>

// 递归遍历目录树, 并对其中每个文件(或目录)调用 fn 函数
int nftw(
  const char *dirpath,
  int (*fn)(const char *fpath,             // 文件路径
            const struct stat *statbuf,    // 文件属性
            int typeflag,                  // 文件类型
            struct FTW *ftwbuf),          // 文件树遍历信息
  int nopenfd,  // 同时打开的文件描述符数量上限, 可以设置成 20
  int flags);
```

flags 参数的取值如下:

- `FTW_PHYS`: 不跟随符号链接
- `FTW_MOUNT`: 仅遍历当前文件系统, 不进入挂载点
- `FTW_CHDIR`: 在遍历时, 会自动切换到当前文件的目录

调用目标函数 fn 时, typeflag 参数的取值如下:

- `FTW_F`: 是个普通文件
- `FTW_D`: 是个目录
- `FTW_DNR`: 是个无法读取的目录(所以无法遍历该目录)
- `FTW_SL`: 是个符号链接
- `FTW_DP`: 是个目录, 且其所有子目录已经被遍历完毕
- `FTW_NS`: 无法获取文件属性
- `FTW_SLN`: 是个符号链接, 但是无法解引用

## 改变进程的根目录: chroot()

chroot() 系统调用可以改变进程的根目录, 使得该进程之后所有的文件操作都以新的根目录为基准.


在一个目录中准备好基础的根文件系统(rootfs) 后, 通过 chroot() 即可以使进程使用这个目录作为根目录, 这就是 Docker 等容器技术的文件
系统隔离的基础.

```c
#include <unistd.h>

int chroot(const char *path);
```

理想情况下, chroot() 会使进程无法访问原来的根目录, 从而实现文件系统隔离. 因此也称这个操作为 chroot jail.

但 chroot() 的实现并没有那么理想, 它的安全性不高, 有许多方法可以逃逸出 jail:

- 特权进程有很多手段可以逃逸出 jail.
- 若 chroot() 之前已经打开了文件描述符, 那么这些文件描述符仍然可以访问到原来的根目录.
- 若在 chroot() 后未调用 chdir() 切换到新的根目录, 那么进程仍然可以通过相对路径访问到原来的根目录.
- ...

因此, NixOS 及 flatpak 等技术都选择了 bubblewrap 这种更安全的隔离方式创建进程沙箱.

