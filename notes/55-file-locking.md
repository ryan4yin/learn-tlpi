# 文件加锁

文件加锁主要是**用于避免多个进程同时读写同一个文件的情况**，但作为一种锁机制，它同样也可用于更通用的需要加锁的场景。

## 使用 flock() 给文件加锁

flock() 可以对一整个文件加锁，它的控制粒度是「文件」。

```c
#include <sys/file.h>

int flock(int fd, int operations);
```

TODO

### 锁继承与释放的语义

TODO

### flock() 的限制

TODO

## 使用 fcntl 给记录枷锁

fcntl() 能够在一个文件的任意部分上放置一把锁，它的粒度更细，可以小到一个字节，也可以大到一整个文件。

使用示例：

```c

```

fnctl() 加锁的类型：

TODO

## 查询系统中存在的文件锁

/proc/locks 文件记录了系统中当前存在的锁，示例：

```bash
$ cat /proc/locks

#序号 锁类型  锁模式   读/写 进程ID 文件系统设备号+i-node  锁start字节 锁的end字节
1:    FLOCK  ADVISORY  WRITE 51497  00:1e:38948904         0 EOF
2:    POSIX  ADVISORY  WRITE 4413   00:1e:1156675          1073741826 1073742335
3:    POSIX  ADVISORY  WRITE 4413   00:1e:4551             1073741826 1073742335
4:    POSIX  ADVISORY  WRITE 4413   00:4c:77               0 0
5:    POSIX  ADVISORY  WRITE 4413   00:4c:78               0 0
6:    FLOCK  ADVISORY  WRITE 3037   00:42:58               0 EOF
7:    POSIX  ADVISORY  WRITE 51497  00:1e:1612398          0 3
8:    POSIX  ADVISORY  WRITE 4413   00:1e:6199248          1073741826 1073742335
9:    POSIX  ADVISORY  READ  8970   00:1d:558              128 128
10:   POSIX  ADVISORY  READ  8970   00:1d:555              1073741826 1073742335
11:   POSIX  ADVISORY  WRITE 4413   00:1e:4216             1073741826 1073742335
12:   POSIX  ADVISORY  WRITE 4413   00:1e:5587358          1073741826 1073742335
13:   POSIX  ADVISORY  WRITE 4413   00:1e:4062             1073741826 1073742335

# 通过进程 ID 查询第一把锁的持有进程相关信息
$ ps -p 51497
    PID TTY          TIME CMD
  51497 tty1     00:02:53 .telegram-deskt
```

`FLOCK` 锁类型表示 flock() 创建的锁，`POSIX` 表示 fcntl 创建的锁。

因为 `FLOCK` 锁只支持锁整个文件，它的起始字节始终为 0，终止字节始终为 EOF.

更直观的方法是使用 util-linux 包提供的 lslocks 命令：

```bash
$ lslocks
COMMAND           PID  TYPE   SIZE MODE  M      START        END PATH
nu               4022 POSIX    36K READ  0 1073741826 1073742335 /home/ryan/.config/nushell/history.sqlite3
nu               4022 POSIX    32K READ  0        128        128 /home/ryan/.config/nushell/history.sqlite3-shm
.firefox-wrappe  4413 POSIX   160K WRITE 0 1073741826 1073742335 /home/ryan/.mozilla/firefox/aeoxkq7d.default/permissions.sqlite
.firefox-wrappe  4413 POSIX   256K WRITE 0 1073741826 1073742335 /home/ryan/.mozilla/firefox/aeoxkq7d.default/content-prefs.sqlite
pipewire         3111 FLOCK        WRITE 0          0          0 /run/user/1000/pipewire-0.lock
pipewire         3111 FLOCK        WRITE 0          0          0 /run/user/1000/pipewire-0-manager.lock
nvim            67851 FLOCK     2B READ  0          0          0 /tmp/nvim.ryan/LsqBs9
```
