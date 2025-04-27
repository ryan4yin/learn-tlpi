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

另外注意在与 fork() 联用时，需要在子进程与父进程都分别调用 `setpgid()` 才能正确修改一个子进程的进程组 ID，并且父进程需要忽略 EACCESS 错误。
这是因为父进程与子进程的调度顺序是无法预知的，因此必须在父子进程都在 fork() 后立即尝试修改子进程的进程组 ID，才能确保该进程组 ID 在 fork() 后被立即修改。

## 会话

```c
#define _XOPEN_SOURCE 500
#include <unistd>

// 获取指定进程的会话 ID
pid_t getsid(pid_t pid);

// 为当前进程创建新会话（若当前进程已经是进程组首进程，则报 EPERM 错误）
pid_t setsid(void);
```

setsid() 系统调用创建新会话的流程：

- 创建新会话
- 调用进程成为「新会话的首进程」以及「该会话中新进程组的首进程」。调用进程的进程组 ID 与会话 ID 都被设置为该进程的进程 ID
- 调用进程没有控制终端，所有之前到控制终端的连接都会在创建新会话时被断开。

## 控制终端与控制进程

控制终端是用于与会话进行交互控制的一个终端（TTY），它对应我们的显示器输出与键鼠输入。

**一个会话只能有一个控制终端**。

如前所述，会话在被创建出来时是没有控制终端的，其中的进程也无法打开 `/dev/tty` 这个设备。

当会话首进程首次打开一个还没有成为某个会话的「控制终端」的「终端」时，会话会创建一个「控制终端」，除非在调用 `open()` 时指定了 `O_NOCTTY` 标记。

第 62 章跟第 64 章会详细介绍终端与伪终端两个概念，这里先略过。

### 1. **终端（Terminal） vs 控制终端（Controlling Terminal）**

- **终端（Terminal）**  
  是一个通用的输入输出环境，可以是：
  - 物理终端（如早期的串行终端设备）。
  - 虚拟终端（如 `tty1`~`tty6`，通过 `Ctrl+Alt+F1~F6` 切换）。
  - 伪终端（PTY，如 SSH 会话或图形终端模拟器如 `gnome-terminal`）。
  - 终端是用户与 Shell 或进程交互的媒介。

- **控制终端（Controlling Terminal）**  
  是进程组（Session）关联的**唯一**终端设备，用于：
  - 接收信号（如 `SIGINT`（Ctrl+C）、`SIGHUP`（终端关闭））。
  - 管理前台/后台进程组（通过 `tcsetpgrp` 设置）。
  - 一个会话（Session）在创建时（通常由 Shell 发起）会绑定一个控制终端，且**只能有一个**。

---

### 2. **一个进程或会话中能否有多个终端？**
- **可以间接实现多终端输入输出**，但需明确设计：
  - 进程可以通过文件操作（如 `open("/dev/ttyX")`）直接读写其他终端设备，但这不是常见的用法。
  - 更常见的多终端交互是通过**伪终端（PTY）**实现的，例如：
    - `screen` 或 `tmux` 工具：它们在单个会话中创建多个虚拟终端（窗口），每个窗口关联不同的 PTY。
    - SSH 多路复用：通过一个 SSH 连接管理多个子会话。
  - 这些工具的核心是通过**多路复用**（Multiplexing）模拟多终端的体验，但底层仍是一个控制终端。

---

### 3. **一个会话能否有多个控制终端？**
- **不能**。这是由 Linux 的会话管理机制决定的：
  - 会话（Session）通过 `setsid()` 创建，最初没有控制终端。
  - 只有会话首进程（通常是 Shell）可以调用 `ioctl(tty_fd, TIOCSCTTY)` 绑定一个控制终端。
  - 一旦绑定，会话中的所有进程组共享同一个控制终端，且无法绑定其他终端。
  - 这种设计保证了信号和进程组管理的唯一性（例如避免 Ctrl+C 发送到多个终端）。

---

### 4. **为什么这样设计？**
- **信号控制的明确性**：确保 `SIGINT`、`SIGHUP` 等信号只发送到当前前台进程组。
- **会话隔离**：防止不同终端的输入/输出混乱（例如两个终端同时尝试读取同一输入）。
- **资源管理**：控制终端是会话的生命周期锚点（终端关闭时会话终止）。

---

### 5. **多个终端的用途**
虽然一个会话只能有一个控制终端，但通过多路复用或工具可以实现多终端的用途：
- **并行任务管理**：如 `tmux` 分屏，同时运行多个任务（编译、日志监控等）。
- **持久化会话**：SSH 断开后通过 `screen` 恢复工作环境。
- **输入输出分离**：将某个进程的输出重定向到另一个终端（如 `echo "test" > /dev/pts/2`）。
- **调试与日志**：将错误日志输出到独立终端以便监控。

---

### 总结
| 特性                | 终端（Terminal）       | 控制终端（Controlling Terminal） |
|---------------------|-----------------------|----------------------------------|
| **数量**            | 可多个                | 每个会话唯一                     |
| **作用**            | 输入输出媒介          | 信号传递、进程组管理             |
| **绑定关系**        | 进程可自由读写        | 会话级绑定，不可更改             |

多终端的实现依赖于工具（如 `tmux`）或直接操作设备文件，而控制终端的唯一性确保了系统的稳定性和一致性。

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


## 孤儿进程组

如果一个父进程在子进程之前终止，那该子进程会被 init 进程收养，进而成为一个孤儿进程。

在 Shell 交互中，Shell 只清楚它所创建的进程状态，并不清楚该进程是否有孤儿子进程仍在运行，
这就可能导致系统后台存在遗留的孤儿子进程。

为了确保这些子进程也在 Shell 退出后正常被清理，在会话终止时系统也会向所有这些孤儿子进程发送一个 SIGHUP 信号。

而上一节「让程序后台运行」则正是采取了手段规避了这一信号，有意地创建了一个在终端退出后仍然正常运行的孤儿子进程。

