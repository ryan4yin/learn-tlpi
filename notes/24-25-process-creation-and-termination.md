# 进程创建与终止


## 进程的创建

- `fork()` 系统调用: 创建一个新进程, 新进程是调用进程的完整副本。新进程被称为子进程, 而调用进程被称为父进程.
- `execve(pathname, argv, envp)` 系统调用: 用一个新程序替换当前进程的程序文本段，并且为新程序初始化栈段、数据段和堆段.
- `wait()` 系统调用: 如果子进程尚未终止, 则挂起调用进程, 直到子进程终止. 如果子进程已经终止，则返回子进程的终止状态，并释放子进程的资源.

在一些系统或者编程语言中，会提供 `spawn()` 函数来创建新进程，这个函数类似于 `fork()` 和 `execve()` 的组合 - 即在子进程中执行一个新程序.


```c
#include <unistd.h>

// 创建一个新进程
pid_t fork(void);
```

## 进程的终止

进程的终止有两种情况：

- 正常终止：进程执行完了它的任务，然后调用 `exit()` 函数来终止自己.
- 异常终止：进程由于某种原因被迫终止，比如调用了一个非法指令，或者接收到一个信号.


首先介绍 `_exit()` 系统调用：

```c
#include <unistd.h>

// 终止调用进程
void _exit(int status);
```

`_exit()` 系统调用会立即终止调用进程，该调用不会返回（因为进程会立即被终止），也不会执行任何清理工作。

我们一般会使用 `exit()` 函数来终止进程，这个函数会在终止进程之前执行一些清理工作，比如关闭文件描述符、释放内存等，最后再调用 `_exit()` 系统调用来终止进程。

```c
#include <stdlib.h>

// 终止调用进程
void exit(int status);
```

`exit()` 函数在终止进程之间 会先干这些事情：

- 调用由 `atexit()` / `on_exit()` 等函数注册的 exit handler 函数.
- 刷新所有 stdio 流的缓存区.
- 调用 `_exit()` 系统调用来终止进程.


### 进程终止的细节

进程不论是正常终止还是异常终止，都会经历以下几个步骤：

* 进程的所有 open file descriptors, directory streams, message catalog decriptors 等各类 fd 与 stream 都会被关闭.
* 所有文件锁都会被释放.
* 所有 System V 共享内存段都会被分离.
* System V Semaphores 相关的操作，没太看明白
* 如果该进程是一个 controlling terminal 的 controlling process, 则一个 SIGHUP 信号会被发送给该进程的 foreground process group, 并且该进程的 controlling terminal 会被断开连接.
* 所有 POSIX named semaphores 都会被关闭.
* 所有 POSIX message queues 都会被关闭.
* 如果因为此进程的终止，某个进程组变成了孤儿并且进程组中有进程处于停止状态，那么这个进程组中的所有进程都会收到一个 SIGHUP 信号并接着收到一个 SIGCONT 信号.
* 任何此进程创建的内存锁都会被移除
* 任何此进程创建的内存映射都会被解除映射


### Exit Handlers

有时候，应用程序会需要在进程终止时执行一些清理工作，比如释放内存、关闭文件描述符等，这就是 exit handler 的功能。

```c
#include <stdlib.h>

// 注册一个 exit handler 函数
int atexit(void (*function)(void));

// 跟 atexit() 类似，但是
//  1. 可以传递一个参数给 exit handler 函数
//  2. exit handler 函数在被调用时会接收到进程的终止状态(int)
int on_exit(void (*function)(int, void *), void *arg);
```

### fork(), stdio buffers 与 _exit() 之间的交互

1. fork() 会复制父进程的 stdio buffers, 导致子进程拥有了父进程 stdio buffers 的副本.
2. 当父子进程在这之后调用 exit() 或 fflush() 时，会导致内容一致的 stdio buffers 被两次刷新到文件中，这会导致输出重复.

解决办法：

1. 父进程在调用 fork() 之前，先调用 fflush() 刷新 stdio buffers，避免子进程继承到脏数据.
2. 在子进程中调用 `_exit()` 来终止进程，而不是调用 `exit()`. 这样子进程就不会执行 stdio buffers 的刷新操作.



