# 监控子进程

> 这一章非常适合配合 [tini](https://github.com/krallin/tini/blob/master/src/tini.c) 的实现来学习, 尤
> 其是 `SIGCHLD` 信号的处理与僵尸进程的处理.

## 等待子进程

> wait & waitpid: https://man7.org/linux/man-pages/man2/wait.2.html

> wait3 & wait4: https://man7.org/linux/man-pages/man2/wait4.2.html

在许多应用中，父进程都需要直到子进程何时终止，以及终止状态是什么。Linux 提供了一系列系统调用来实现这
一目的。

```c
#include <sys/wait.h>

// 等待某一个子进程终止，返回子进程 pid，并将子进程的终止状态存储在 status 中
pid_t wait(int *status);

// 与 wait() 类似，但是可以指定等待的子进程 pid，并且可以指定一些选项来控制等待的行为
pid_t waitpid(pid_t pid, int *status, int options);

// 与 waitpid() 类似，但是可以更精确地指定等待的子进程类型
pid_t waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);

// 与 waitpid() 类似，但是可以获取子进程的资源使用情况
// wait3 不能指定子进程的 pid.
// 函数名称来源：它有 3 个参数
pid_t wait3(int *_Nullable wstatus, int options, struct rusage *_Nullable rusage);
// 与 wait3() 类似，但可以指定等待的子进程 pid
// 函数命名来源：它有 4 个参数
pid_t wait4(pid_t pid, int *_Nullable wstatus, int options, struct rusage *_Nullable rusage);
```

### wait() 系统调用

wait() 的工作流程：

## 孤儿与僵尸进程

> https://man7.org/linux/man-pages/man2/wait.2.html#NOTES

- **A child that terminates, but has not been waited for becomes a "zombie"**.
  - The kernel maintains a minimal set of information about the zombie process (PID, termination
    status, resource usage information) in order to allow the parent to later perform a wait to
    obtain information about the child.
  - As long as a zombie is not removed from the system via a wait, it will consume a slot in the
    kernel process table, and if this table fills, it will not be possible to create further
    processes.


产生僵尸进程的一些常见场景：

- docker 容器：容器的 1 号进程如果是普通的应用程序，它很可能未实现对僵尸子进程的处理，导致容器内的僵尸进程
  无法被回收.
  - docker 在新版本中已经自带了 tini 用于处理这个问题.
- shell 脚本中的后台进程：shell 脚本如果在创建后台进程后直接退出，那么这些后台进程在退出后会变成僵尸进程.
- 在编写各类代码时，如果父进程一味地创建子进程（比如执行外部程序），却没有使用 wait() / join() 等函数来等待子进程的终止，那么这些
  子进程就会变成僵尸进程.
- ...

