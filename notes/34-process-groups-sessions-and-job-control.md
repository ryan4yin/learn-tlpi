# 进程组, 会话和 Job 控制

会话、进程组的主要用途是用于 Shell 的 Job 控制，不过现在可能在 tmux/zellij 这样的终端复用器中更常用。

- 进程组是一组相关进程的集合，进程的 PGID 为其进程组标识符
  - 创建新进程组的进程为该进程组的**首进程**，其进程 ID 即为该进程组的 PGID
  - 新建进程会继承其父进程的 PGID
  - 首进程可以提前退出进程组，因此可能进程组中首进程不存在的情况
- 会话是一组相关进程组的集合，进程的会话标识符 SID 标明了其所属会话
  - 创建新会话的进程是该会话的**会话首进程**，其进程 ID 会成为会话 ID
  - 新建进程会继承父进程的会话 ID
  - **一个会话中的所有进程共享单个控制终端**
  - 会话中的进程组分为**前台进程组**与后台进程组，任一时刻都只有一个进程组可以位于前台
  - 只有前台进程组才可以从控制终端读取输入

## 进程组

进程可以很简单的在不同进程组间移动，将进程移动到一个新的进程组也是可行的（该进程将成为新进程组的首进程）。

```c
#include <unistd.h>

// 获取当前进程的进程组 ID.
pid_t getpgrp(void);

// 修改某进程的进程组 ID
// pid 为 0，则表示修改当前进程的进程组 ID
// pgid 为 0, 则表示以当前进程 ID 为进程组 ID
// setpgid(0, 0) -> 将当前进程设置为进程组的首进程（将 PGID 修改为其自身进程 ID）
pid_t setpgid(pid_t pid, pid_t pgid);
```

`setpgid()` 功能这么强大，当然也存在一些使用限制：

- pid 目标进程必须为调用进程或其子进程。否则会导致 ESRCH 错误。
- 在组之间移动进程时，调用进程、被移动的目标进程以及目标进程组必须要属于同一个会话。否则会导致 EPERM 错误。
- 不能修改会话首进程的进程组 ID，也即 pid 参数不能为首进程 ID. 否则会导致 EPERM 错误。
- 无法修改已经执行了 exec() 函数的子进程的进程组 ID. 否则会导致 EACCES 错误。
  - 这是因为在一个进程开始执行后再修改其进程组 ID 很可能导致程序混乱（即它通过 getpgrp() 拿到的 ID 可能会不是正确 ID）。

## 如何运行一个在 Shell 退出后仍能继续运行的进程?

Linux 系统管理员应该基本都知道这个问题的答案:

```bash
# 方法一: 使用 nohup
nohup <your command> &

# 方法二: stop + bg + disown
## 1. 首先执行你的命令
<your command>
## 2. 然后按下 Ctrl + Z 暂停进程
## 3. 使用 bg 命令将进程放到后台运行
bg
## 4. 使用 disown 命令将进程从 shell 的作业列表中移除
disown
## 5. 最后退出 shell


# 方法三: 使用 setsid
# NOTE: 注意在使用 setsid 时, 如果未指定 --fork 参数, 则不能在末尾加上 & 符号!
# 末尾添加 & 会导致此命令失效, 原因参见
#   https://stackoverflow.com/questions/21231970/why-setsid-could-not-exit-from-shell-script
setsid --fork <your command>

# 方法四: 使用 screen / tmux 等多路复用工具

# 最佳实践: 对于需要长期执行的后台任务，建议使用 systemd 或 supervisord 等进程管理工具
```

但在我学习本书前, 我只是会使用这些方法, 并不理解 nohup/setsid/disown 的工作原理.

要理解这些命令, 关键在于这几点:

1. 当连接一个终端时, login shell 会成为 session leader, 进而成为该终端的 controlling process.
1. 当终端断开连接时, 该终端的 controlling process (shell) 会收到 SIGHUP 信号.
1. 默认情况下, shell 在收到 SIGHUP 信号后会将该信号传递给它的子进程.
1. 根据第 20 章对信号的描述, SIGHUP 信号的默认处理方式是终止进程, 因此子进程会被终止.

作为例证, 如下是 bash 对 SIGHUP 信号的描述:

> The shell exits by default upon receipt of a SIGHUP. Before exiting, **an interactive shell
> resends the SIGHUP to all jobs, running or stopped**. Stopped jobs are sent SIGCONT to ensure that
> they receive the SIGHUP. To prevent the shell from sending the SIGHUP signal to a particular job,
> it should be removed from the jobs table with the disown builtin (see Job Control Builtins) or
> marked to not receive SIGHUP using disown -h.

下面逐个解析这几个命令:

- `nohup`: 在运行程序时, 忽略 SIGHUP 信号, 即不会将 SIGHUP 信号传递给它创建的子进程.
  - 这样就可以避免在退出 shell 后, 程序被 SIGHUP 信号终止.
  - 由于 nohup 仍旧在前台运行, 因此需要在命令末尾加上 `&` 符号, 将其放到后台运行.
- `setsid`: 创建一个新的 session, 并将调用进程设置为该会话的 leader.
  - 根据 `man setsid`, 默认情况下 `setsid` 只会在其自身是 process group leader 时才会使用 fork() 创
    建子进程. 而在 shell 中如果你在末尾加上 `&` 符号, 则 shell 会将该进程放到后台运行, 并且 shell 进
    程会成为该进程的 process group leader. 这就会导致 `setsid` 直接在当前进程中执行命令, 从而无法创
    建新的 session.
