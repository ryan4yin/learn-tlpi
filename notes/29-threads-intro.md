# 线程 - 介绍

> 注意，这里介绍的是 OS 级别的线程，与编程语言中的线程概念有所不同。不同的编程语言会有自己的线程实现，它们与 OS 线程的关系取决于具体的实现。

接下来的几章介绍 POSIX threads API，它也常被称为 Pthreads API.

在 Linux 中，Pthreads API 存在两套实现：LinuxThreads 和 NPTL（Native POSIX Threads Library）。

LinuxThreads 是最早的实现，它是一个用户空间线程库，其主要问题是性能差，早已不再被 glibc 支持。

NPTL 则是 Pthreads API 的新实现，它充分利用了 Linux 内核的线程支持，性能更好，也更符合 POSIX 标准。

## Pthreads Overview

> https://man7.org/linux/man-pages/man7/pthreads.7.html

在 Linux 内核中, 线程就是一种轻量的进程, 其创建速度通常比进程快 10 倍以上.

[Four threads executing in a process (Linux/x86-32)](../_img/29-1-memory-of-4-threads-in-a-process.png)

与进程的主要区别是, 它们之间共享了相同的:

- 全局内存空间
- 进程 ID 与父进程 ID
- 进程组 ID 与会话 ID
- 控制终端
- 进程凭证
- 打开的文件描述符
- 由 fnctl 创建的记录锁(record lock)
- 信号处置
- 文件系统相关信息: 文件权限, 当前工作目录, 根目录.
- 间隔定时器(setitimer与 POSIX 定时器(timer_create)
- System V 信号量撤销(undo, semadj)
- 资源限制(setrlimit)
- CPU 时间消耗(由 times() 返回)
- 资源消耗(由 getrusage() 返回)
- nice 值(由 setpriority() 和 nice() 设置)

各线程独有的属性包括:

- 线程 ID
- 信号掩码
- 线程特有数据(TSD, Thread-Specific Data)
- 备选信号栈(signalstack)
- errno 变量
- 浮点型环境(fenv)
- 实时调度策略与优先级
- CPU 亲和力
- 能力(capability)
- 栈, 本地变量, 函数的调用链接信息
- ...

> NOTE: 前面的图中能看到，所有线程的栈空间实际是在同一虚拟地址空间中的，所以只要能拿到合适的内存指
> 针，线程之间就能相互访问彼此的栈空间。这也是为什么多线程编程中需要特别小心的原因之一。

## Pthreads API 基础概念

关键数据类型：

| Data type           | Description                             |
| ------------------- | --------------------------------------- |
| pthread_t           | Thread identifier                       |
| pthread_mutex_t     | Mutex                                   |
| pthread_mutexattr_t | Mutex attributes object                 |
| pthread_cond_t      | Condition variable                      |
| pthread_condattr_t  | Condition variable attributes object    |
| pthread_key_t       | Key for thread-specific data            |
| pthread_once_t      | One-time initialization control context |
| pthread_attr_t      | Thread attributes object                |

### 编译

在 Linux 上，编译使用了 Pthreads API 程序时，需要使用 `cc -pthread` 参数，该参数的功能：

- 添加 `-lpthread` 选项，以链接 libpthread 库
- 添加 `-D_REENTRANT` 宏定义


## 线程的创建

> https://man7.org/linux/man-pages/man3/pthread_create.3.html

```c
#include <pthread.h>

// 创建一个新线程
//   thread: 如果调用成功，函数会将新线程的 ID 存储在此指针指向的内存中
//   attr: 线程属性，通常为 NULL，表示使用默认属性
//   start_routine: 新线程的入口函数，它只有一个 void* 类型的参数，返回值也是 void*
//   arg: 传递给新线程入口函数的参数
int pthread_create(pthread_t *restrict thread,
                  const pthread_attr_t *restrict attr,
                  void *(*start_routine)(void *),
                  void *restrict arg);
```

> 这里的参数使用了 void * 类型，其目的跟之前介绍的 clone() 函数类似，都是为了能够传递任意类型的参数。


## 线程的终止

> https://man7.org/linux/man-pages/man3/pthread_exit.3.html

```c
#include <pthread.h>

// 终止调用线程
//  retval: 线程的返回值，注意此指针不能指向一个线程栈上的地址，因为线程终止后，其栈空间会被释放
[[noreturn]] void pthread_exit(void *retval);
```

线程可能以如下几种方式终止：

- 线程执行完入口函数 start_routine 后 return 一个值。
- 线程调用 `pthread_exit()`, 此函数的效果与线程函数 return 的效果相同，区别在于它可以在线程的任何地方调用。
- 线程被其他线程调用 `pthread_cancel()` 取消。
- 任一线程调用 `exit()` 或 `_exit()` 终止进程，这会立即终止整个进程以及其中的所有线程。


## 线程 ID

```c
#include <pthread.h>

// 获取当前线程的 ID
pthread_t pthread_self(void);

// 比较两个线程 ID 是否相等
// NOTE: 不应假定线程 ID 的具体实现，因此不能直接使用 == 来比较线程 ID，需要使用此函数
int pthread_equal(pthread_t t1, pthread_t t2);
```


## 等待线程的终止

```c
#include <pthread.h>

// 等待线程的终止
//   thread: 等待的线程 ID
//   retval: 线程的返回值
int pthread_join(pthread_t thread, void **retval);
```

`pthread_join()` 函数会阻塞调用线程，直到指定的线程终止。如果线程已经终止，那么函数会立即返回。

如果 retval 不为 NULL，那么线程的返回值会存储在 *retval 中。

> NOTE: 如果 `pthread_join` 一个已经被 join 过的线程，可能会导致未定义行为。因此请确保每个线程只被 join 一次。

`pthread_join()` 基本就是 `waitpid()` 的线程版本，但它也存在一些特别之处：

- Threads 之间的关系是对等的，没有父子关系。
  - 任何线程创建的新线程，都与其他所有线程（包括该线程的创建者）处于同一层级。
  - 每个线程都可以使用 `pthread_join()` 来等待其他任何线程的终止。
- 不存在「等待任意线程」的接口，也无法以非阻塞的方式等待线程的终止。
  - 这是有意为之的，因为线程之间没有层级关系，「线程只应该被允许等待它已知的其他线程」。


## detach 一个线程

默认情况下，线程是 joinable 的，这意味着即使线程已经终止，其终止状态和返回值仍然会被保留，直到其他线程调用 `pthread_join()` 来获取。

但是，有时候我们并不关心线程的终止状态，或者不希望其他线程等待它的终止。这时可以将线程设置为 detached 状态。

```c
#include <pthread.h>

int pthread_detach(pthread_t thread);
```

譬如 `pthread_detach(pthread_self());` 可以将当前线程自身设置为 detached 状态。

- 被 detach 的线程不可以被 join，也无法再被改回到 joinable 状态。
- detached 线程终止后，其资源会被自动回收。
- detached 线程仍旧受其他线程调用 `exit()` 或 `_exit()` 终止进程的影响。



## 线程 vs 进程


线程的优势：

- 线程间通信更加简单，因为它们共享相同的地址空间。
- 创建线程的开销更小，因为线程共享了大部分资源。

线程的劣势：

- 因为线程间共享了相同的地址空间，所以出现了「线程安全（thread-safe）」问题，需要非常小心地处理，这也是并发编程中最大的挑战之一。而在进程中因为数据是隔离的，所以不存在这个问题。
- 还是因为线程间共享了内存空间与其他属性，一个线程的 bug 可能会摧毁其他所有线程，而进程的 bug 只会影响到自己。

其他问题：

- 线程与信号天然不和，在多线程程序中，信号处理需要格外小心。
  - 最好就不要在多线程程序中使用信号
- 线程间不仅共享内存空间，还共享了文件描述符、当前工作目录等很多属性，视情况而定，这可能是优势也可能是劣势。


