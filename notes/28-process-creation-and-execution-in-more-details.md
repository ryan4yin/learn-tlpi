# 详述进程创建与执行

## 进程记账

> https://man7.org/linux/man-pages/man5/acct.5.html

> 对于比较容易复现的应用程序自身的问题而言, 使用 `strace` 和 `perf` 等工具来定位问题, 可能比进程记账
> 更加方便.

Linux 提供了进程记账功能, 该功能默认是关闭的, 可以通过 `acct()` 系统调用来开启.

启用进程记账功能后, 内核会在进程退出时, 将进程的记账信息写入到 `acct()` 系统调用指定的文件中. 这个记
账文件的路径通常是 `/var/log/pacct`.

进程记账功能的主要作用是统计系统中各个进程的资源使用情况, 以便后续的性能分析和优化.

## clone() 函数

> https://man7.org/linux/man-pages/man2/clone.2.html

类似 fork() 系统调用, Linux 特有的 clone() 函数也用于创建新进程. 区别在于, clone() 函数可以通过
flags 参数来控制新进程与父进程之间的资源共享程度.

在现代程序中, clone() 函数可能比 fork() 系统调用更常用, 因为 clone() 的性能通常比 fork() 系统调用更
高.

clone() 也是容器隔离技术的基础, 它可以通过 flags 控制新进程是否使用独立的各类名字空间, 从而实现容器
隔离.

```c
#define _GNU_SOURCE
#include <sched.h>

// 创建新进程
// 第一个函数参数是新进程的入口函数
// 第二个参数是新进程的栈空间顶部指针
// 第三个参数是 flags, 用于控制新进程与父进程之间的资源共享程度
// 第四个参数是传递给新进程入口函数的参数
int clone(int (*fn)(void *_Nullable), void *stack, int flags,
          void *_Nullable arg, ...  /* pid_t *_Nullable parent_tid,
                                      void *_Nullable tls,
                                      pid_t *_Nullable child_tid */ );
```

> C 标准中实际并未对将数据从 void * 与 int 等具体类型之间的强制转换做出规定，但大部分 C 编译器都能保证 `int j == (int) ((void *) j)` 这种转换是安全的.
> clone() 函数就是利用了这一点, 通过 void * 类型的参数传递任意类型的参数. 

因为 clone() 函数创建的新进程可能与父进程共享内存空间, 因此它不能直接使用父进程的栈空间(那样会导致父
进程的栈空间被破坏). 因此, 在调用 clone() 函数时, 需要为新进程分配一个新的栈空间, 该栈空间通常是通过
`malloc()` 函数动态分配的, 并通过 `clone()` 函数的第二个参数传递给新进程.

另有一个缺陷是, 栈空间指针是向下增长的, 因此在调用 clone() 函数时, 需要将栈空间指针指向栈空间的顶部.
不能直接提供 `malloc()` 函数返回的指针, 因为它是栈空间的底部地址.

flags 参数是一个位掩码, 其可选值参见: https://man7.org/linux/man-pages/man2/clone.2.html

clone() 实质是 glibc 对 Linux 内核 `sys_clone()` 系统调用的封装函数. 在 Linux 内核中, `sys_clone()`
与 `fork()` 系统调用最终都由同一函数 `do_fork()` 实现.

### 为什么说 `fork()` 是创建进程，而 `clone()` 是创建线程

> fork() 是系统调用, 而 clone() 是 glibc 基于 `sys_clone()` 系统调用的封装函数.

某种意义上, 对术语 "进程" 和 "线程" 的区分不过是在玩弄文字游戏而已. 从内核的角度来看, 它们都是 KSE
(Kernel Scheduling Entity, 内核调度实体), 区别在于它们与其他 KSE 之间对属性(虚拟内存、打开文件描述
符、对信号的处置、进程 ID 等)的共享程度不同.

一句话解释，进程之间的资源是独立的, 而**线程只是 Linux 中一类特殊的进程**, 线程之间能共享许多资源.

- 进程: fork() 调用创建的新进程复制(CoW 写时复制)了父进程的栈段、数据段和堆段, 只有文本段(代码)是共
  享的.
- 线程: clone() 函数可以通过 flags 参数来细粒度控制新建进程与父进程之间的资源共享程度.

**在使用 clone() 创建进程时, 进程之间共享的资源够越多, clone() 的执行速度就越快**, clone() 的性能通
常远高于 fork().

目前 Linux 上主流的线程实现 NPTL 就是基于 clone() 实现的, 它底层就是通过 clone() 来创建共享资源足够
多的进程, 以实现线程的功能.

虽然 NPTL 是 glibc 的一部分，但它与 Linux 内核紧密协作，以提供符合 POSIX 标准的线程功能。NPTL 的设计
旨在利用内核提供的线程能力，以实现高效和兼容的线程操作。这意味着 NPTL 的实现既是用户空间的（通过
glibc），也依赖于内核空间的特性。

后面的 29 - 33 章会江西讲解线程的使用及其底层实现.

## execve() 和 for() 对进程属性的影响

Process address space:

| Process attribute      | exec()    | fork()         | Interfaces affecting attribute; additional notes                                                                                                                                     |
| ---------------------- | --------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Text segment           | No        | Shared         | Child process shares text segment with parent.                                                                                                                                       |
| Stack segment          | No        | Yes            | Function entry/exit; `alloca()`, `longjmp()`, `siglongjmp()`.                                                                                                                        |
| Data and heap segments | No        | Yes            | `brk()`, `sbrk()`.                                                                                                                                                                   |
| Environment variables  | See notes | Yes            | `putenv()`, `setenv()`; direct modification of `environ`. Overwritten by `execle()` and `execve()` and preserved by remaining `exec()` calls.                                        |
| Memory mappings        | No        | Yes; see notes | `mmap()`, `munmap()`. A mapping’s `MAP_NORESERVE` flag is inherited across `fork()`. Mappings that have been marked with `madvise(MADV_DONTFORK)` are not inherited across `fork()`. |
| Memory locks           | No        | No             | `mlock()`, `munlock()`.                                                                                                                                                              |

Process identifiers and credentials:

| Process attribute           | exec()    | fork() | Interfaces affecting attribute; additional notes                                             |
| --------------------------- | --------- | ------ | -------------------------------------------------------------------------------------------- |
| Process ID                  | Yes       | No     |                                                                                              |
| Parent process ID           | Yes       | No     |                                                                                              |
| Process group ID            | Yes       | Yes    | `setpgid()`.                                                                                 |
| Session ID                  | Yes       | Yes    | `setsid()`.                                                                                  |
| Real IDs                    | Yes       | Yes    | `setuid()`, `setgid()`, and related calls.                                                   |
| Effective and saved set IDs | See notes | Yes    | `setuid()`, `setgid()` and related calls. Chapter 9 explains how `exec()` affects these IDs. |
| Supplementary group IDs     | Yes       | Yes    | `setgroups()`, `initgroups()`.                                                               |

Files, file I/O, and directories:

| Process attribute           | exec()       | fork()         | Interfaces affecting attribute; additional notes                                                                                                                                                                                  |
| --------------------------- | ------------ | -------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Open file descriptors       | See notes    | Yes            | `open()`, `close()`, `dup()`, `pipe()`, `socket()`, and so on. File descriptors are preserved across `exec()` unless marked close-on-exec. Descriptors in child and parent refer to same open file descriptions; see Section 5.4. |
| Close-on-exec flag          | Yes (if off) | Yes            | `fcntl(F_SETFD)`.                                                                                                                                                                                                                 |
| File offsets                | Yes          | Shared         | `lseek()`, `read()`, `write()`, `readv()`, `writev()`. Child shares file offsets with parent.                                                                                                                                     |
| Open file status flags      | Yes          | Shared         | `open()`, `fcntl(F_SETFL)`. Child shares open file status flags with parent.                                                                                                                                                      |
| Asynchronous I/O operations | See notes    | No             | `aio_read()`, `aio_write()`, and related calls. Outstanding operations are canceled during an `exec()`.                                                                                                                           |
| Directory streams           | No           | Yes; see notes | `opendir()`, `readdir()`. SUSv3 states that child gets a copy of parent’s directory streams, but these copies may or may not share the directory stream position. On Linux, the directory stream position is not shared.          |

File system:

| Process attribute         | exec() | fork() | Interfaces affecting attribute; additional notes |
| ------------------------- | ------ | ------ | ------------------------------------------------ |
| Current working directory | Yes    | Yes    | `chdir()`.                                       |
| Root directory            | Yes    | Yes    | `chroot()`.                                      |
| File mode creation mask   | Yes    | Yes    | `umask()`.                                       |

Signals:

| Process attribute      | exec()    | fork() | Interfaces affecting attribute; additional notes                                                                                                                                        |
| ---------------------- | --------- | ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Signal dispositions    | See notes | Yes    | `signal()`, `sigaction()`. During an `exec()`, signals with dispositions set to default or ignore are unchanged; caught signals revert to their default dispositions. See Section 27.5. |
| Signal mask            | Yes       | Yes    | Signal delivery, `sigprocmask()`, `sigaction()`.                                                                                                                                        |
| Pending signal set     | No        | No     | Signal delivery; `raise()`, `kill()`, `sigqueue()`.                                                                                                                                     |
| Alternate signal stack | No        | Yes    | `sigaltstack()`.                                                                                                                                                                        |

Timers:

| Process attribute       | exec() | fork() | Interfaces affecting attribute; additional notes |
| ----------------------- | ------ | ------ | ------------------------------------------------ |
| Interval timers         | Yes    | No     | `setitimer()`.                                   |
| Timers set by `alarm()` | Yes    | No     | `alarm()`.                                       |
| POSIX timers            | No     | No     | `timer_create()` and related calls.              |

POSIX Threads:

| Process attribute                   | exec() | fork()    | Interfaces affecting attribute; additional notes                                                                                     |
| ----------------------------------- | ------ | --------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| Threads                             | No     | See notes | During `fork()`, only calling thread is replicated in child.                                                                         |
| Thread cancelability state and type | No     | Yes       | After an `exec()`, the cancelability type and state are reset to `PTHREAD_CANCEL_ENABLE` and `PTHREAD_CANCEL_DEFERRED`, respectively |
| Mutexes and condition variables     | No     | Yes       | See Section 33.3 for details of the treatment of mutexes and other thread resources during `fork()`.                                 |

Priority and scheduling:

| Process attribute              | exec() | fork() | Interfaces affecting attribute; additional notes |
| ------------------------------ | ------ | ------ | ------------------------------------------------ |
| Nice value                     | Yes    | Yes    | `nice()`, `setpriority()`.                       |
| Scheduling policy and priority | Yes    | Yes    | `sched_setscheduler()`, `sched_setparam()`.      |

Resources and CPU time

| Process attribute           | exec() | fork() | Interfaces affecting attribute; additional notes |
| --------------------------- | ------ | ------ | ------------------------------------------------ |
| Resource limits             | Yes    | Yes    | `setrlimit()`.                                   |
| Process and child CPU times | Yes    | No     | As returned by `times()`.                        |
| Resource usages             | Yes    | No     | As returned by `getrusage()`.                    |

Interprocess communication

| Process attribute               | exec()    | fork()    | Interfaces affecting attribute; additional notes                                                                                                                                       |
| ------------------------------- | --------- | --------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| System V shared memory segments | No        | Yes       | `shmat()`, `shmdt()`.                                                                                                                                                                  |
| POSIX shared memory             | No        | Yes       | `shm_open()` and related calls.                                                                                                                                                        |
| POSIX message queues            | No        | Yes       | `mq_open()` and related calls. Descriptors in child and parent refer to same open message queue descriptions. A child doesn’t inherit its parent’s message notification registrations. |
| POSIX named semaphores          | No        | Shared    | `sem_open()` and related calls. Child shares references to same semaphores as parent.                                                                                                  |
| POSIX unnamed semaphores        | No        | See notes | `sem_init()` and related calls. If semaphores are in a shared memory region, then child shares semaphores with parent; otherwise, child has its own copy of the semaphores.            |
| System V semaphore adjustments  | Yes       | No        | `semop()`. See Section 47.8.                                                                                                                                                           |
| File locks                      | Yes       | See notes | `flock()`. Child inherits a reference to the same lock as parent.                                                                                                                      |
| Record locks                    | See notes | No        | `fcntl(F_SETLK)`. Locks are preserved across `exec()` unless a file descriptor referring to the file is marked close-on-exec; see Section 55.3.5.                                      |

Miscellaneous

| Process attribute          | exec() | fork() | Interfaces affecting attribute; additional notes                                                                                           |
| -------------------------- | ------ | ------ | ------------------------------------------------------------------------------------------------------------------------------------------ |
| Locale settings            | No     | Yes    | `setlocale()`. As part of C run-time initialization, the equivalent of `setlocale(LC_ALL, “C”)` is executed after a new program is execed. |
| Floating-point environment | No     | Yes    | When a new program is execed, the state of the floating-point environment is reset to the default; see `fenv(3)`.                          |
| Controlling terminal       | Yes    | Yes    |                                                                                                                                            |
| Exit handlers              | No     | Yes    | `atexit()`, `on_exit()`.                                                                                                                   |

Linux-specific

| Process attribute              | exec()    | fork()    | Interfaces affecting attribute; additional notes                                                                                      |
| ------------------------------ | --------- | --------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| File-system IDs                | Yes       | Yes       | `setfsuid()`, `setfsgid()`. These IDs are also changed any time the corresponding effective IDs are changed.                          |
| timerfd timers                 | Yes       | See notes | `timerfd_create()`; child inherits file descriptors referring to same timers as parent.                                               |
| Capabilities                   | See notes | Yes       | `capset()`. The handling of capabilities during an `exec()` is described in Section 39.5.                                             |
| Capability bounding set        | Yes       | Yes       |                                                                                                                                       |
| Capabilities securebits flags  | See notes | Yes       | All securebits flags are preserved during an `exec()` except `SECBIT_KEEP_CAPS`, which is always cleared.                             |
| CPU affinity                   | Yes       | Yes       | `sched_setaffinity()`.                                                                                                                |
| SCHED_RESET_ON_FORK            | Yes       | No        | See Section 35.3.2.                                                                                                                   |
| Allowed CPUs                   | Yes       | Yes       | See cpuset(7).                                                                                                                        |
| Allowed memory nodes           | Yes       | Yes       | See cpuset(7).                                                                                                                        |
| Memory policy                  | Yes       | Yes       | See set_mempolicy(2).                                                                                                                 |
| File leases                    | Yes       | Yes       | fcntl(F_SETLEASE). Child inherits a reference to the same lease as parent.                                                            |
| Directory change notifications | Yes       | No        | The dnotify API, available via fcntl(F_NOTIFY).                                                                                       |
| prctl(PR_SET_DUMPABLE)         | See notes | Yes       | During an exec(), the PR_SET_DUMPABLE flag is set, unless execing a set-user-ID or set-group-ID program, in which case it is cleared. |
| prctl(PR_SET_PDEATHSIG)        | Yes       | No        |                                                                                                                                       |
| prctl(PR_SET_NAME)             | No        | Yes       |                                                                                                                                       |
| oom_adj                        | Yes       | Yes       | See Section 49.9.                                                                                                                     |
| coredump_filter                | Yes       | Yes       | See Section 22.1.                                                                                                                     |
