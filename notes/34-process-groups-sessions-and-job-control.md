# 进程组, 会话和 Job 控制

- 进程组是一组相关进程的集合.
- 会话是一组相关进程组的集合.

## 进程组



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
