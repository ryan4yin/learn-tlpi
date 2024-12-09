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
// 它等同于 waitpid(-1, &status, options), 只是多了一个 rusage 参数
pid_t wait3(int *_Nullable wstatus, int options, struct rusage *_Nullable rusage);
// 与 wait3() 类似，但可以指定等待的子进程 pid
// 函数命名来源：它有 4 个参数
// 它等同于 waitpid(pid, &status, options), 只是多了一个 rusage 参数
pid_t wait4(pid_t pid, int *_Nullable wstatus, int options, struct rusage *_Nullable rusage);
```

### wait() 系统调用

wait() 的工作流程：

- 如果被调用时已经有终止的子进程，那么立即返回.
- 如果没有终止的子进程，那么父进程会被阻塞，直到有子进程终止.
- 如果 status 参数不为 NULL, 则会将子进程的终止状态存储在 status 中.
- 内核会将父进程下所有子进程的运行总量追加到进程 CPU 时间以及资源使用数据中.ｄｄ
- 如果被调用时并可被等待的子进程, 则返回 -1, 并设置 errno 为 ECHILD.

返回值 status 是一个整数，包含了子进程的终止状态，`<sys/wait.h>` 头文件中定义了一些宏来解析这个整数：

- WIFEXITED(status): 如果子进程正常终止，返回 true.
  - 此时可以使用 WEXITSTATUS(status) 来获取子进程的退出状态.
- WIFSIGNALED(status): 如果子进程是因为信号而终止，返回 true.
  - 此时可以使用 WTERMSIG(status) 来获取终止子进程的信号.
- WIFSTOPPED(status): 如果子进程是因为信号而暂停，返回 true.
- WIFCONTINUED(status): 如果子进程是因为 SIGCONT 而继续运行，返回 true.
- WCOREDUMP(status): 如果子进程产生了 core dump 文件，返回 true.

### waitpid(), waitid() 系统调用

waitpid() 函数的参数说明：

- pid:
  - 若 pid > 0, 则等待进程 ID 为 pid 的子进程.
  - 若 pid = 0, 则等待与调用进程属于同一个进程组的任意子进程.
  - 若 pid = -1, 则等待任意子进程. `waitpid(-1, &status, 0)` 等价于 `wait(&status)`.
  - 若 pid < -1, 则等待进程组 ID 为 pid 绝对值的任意子进程.
- options: 一个位掩码参数，可以指定一些选项
  - WUNTRACED: 不仅返回已经终止的子进程，还返回因为信号而暂停的子进程.
  - WCONTINUED: 返回因为 SIGCONT 而继续运行的子进程.
  - WNOHANG: 即不阻塞，如果没有满足条件的子进程，立即返回, 返回值为 0.

相比 waitpid(), waitid() 可以更精确地指定等待的子进程类型，它的第一个参数 idtype_t 可以是：

- P_ALL: 等待任意子进程, 同时忽略 id 参数.
- P_PID: 等待进程 ID 与 id 参数相同的子进程.
- P_PGID: 等待进程组 ID 与 id 参数相同的子进程.

waitid() 与 waitpid() 最显著的区别就在于其 options 参数设计得更精细, waitid() 的 options 参数可以是：

- WEXITED: 等待已经终止的子进程.
- WSTOPPED: 等待因为信号而暂停的子进程.
- WCONTINUED: 等待因为 SIGCONT 而继续运行的子进程.
- WNOHANG: 与 waitpid() 的 WNOHANG 同义, 即不阻塞.
- WNOWAIT: 不删除子进程的状态信息，子进程的状态信息可以被其他进程等待.

如果 waitid() 调用成功, 则会将子进程的信息存储在其 infop 参数指向的 siginfo_t 结构体中, 其中包含了子进程的 PID, 终止状态, 信号等信息.
该结构体的定义在 sigaction 函数的文档中有描述:

> https://man7.org/linux/man-pages/man2/sigaction.2.html

```c
siginfo_t {
    int      si_signo;     /* Signal number */
    int      si_errno;     /* An errno value */
    int      si_code;      /* Signal code, CLD_EXITED / CLD_KILLED / CLD_STOPPED / CLD_CONTINUED */
    int      si_trapno;    /* Trap number that caused
                              hardware-generated signal
                              (unused on most architectures) */
    pid_t    si_pid;       /* Sending process ID */
    uid_t    si_uid;       /* Real user ID of sending process */
    int      si_status;    /* Exit value or signal */
    clock_t  si_utime;     /* User time consumed */
    clock_t  si_stime;     /* System time consumed */
    union sigval si_value; /* Signal value */
    int      si_int;       /* POSIX.1b signal */
    void    *si_ptr;       /* POSIX.1b signal */
    int      si_overrun;   /* Timer overrun count;
                              POSIX.1b timers */
    int      si_timerid;   /* Timer ID; POSIX.1b timers */
    void    *si_addr;      /* Memory location which caused fault */
    long     si_band;      /* Band event (was int in
                              glibc 2.3.2 and earlier) */
    int      si_fd;        /* File descriptor */
    short    si_addr_lsb;  /* Least significant bit of address
                              (since Linux 2.6.32) */
    void    *si_lower;     /* Lower bound when address violation
                              occurred (since Linux 3.19) */
    void    *si_upper;     /* Upper bound when address violation
                              occurred (since Linux 3.19) */
    int      si_pkey;      /* Protection key on PTE that caused
                              fault (since Linux 4.6) */
    void    *si_call_addr; /* Address of system call instruction
                              (since Linux 3.5) */
    int      si_syscall;   /* Number of attempted system call
                              (since Linux 3.5) */
    unsigned int si_arch;  /* Architecture of attempted system call
                              (since Linux 3.5) */
}
```

### wait3(), wait4() 系统调用

前面简单介绍过了, 它们主要就是多了一个 rusage 参数, 用于获取子进程的资源使用情况.

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


## SIGCHLD 信号

父进程无法预知其子进程何时终止，因此内核会在子进程终止时向父进程发送一个 SIGCHLD 信号，父进程可以通过为此信号
注册一个信号处理函数来处理子进程的终止.

要注意的是, 在 SIGCHLD 的信号处理函数中, 调用 waitpid() 等函数时, 一定要使用 WNOHANG 选项来获取子进程的终止状态,
否则会导致父进程阻塞.


