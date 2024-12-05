# 定时器与休眠

## 传统的 UNIX 间隔定时器

```c
#include <sys/time.h>

// 设置或修改某一类型的定时器
int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);

// 查询某一类型的定时器
int getitimer(int which, struct itimerval *curr_value);
```

`setitimer()` 函数用于设置一段时间后触发的定时器, 也可选择使该定时器周期性触发.

其 `whcih` 参数可取值为:

- `ITIMER_REAL`: 以系统真实时间计算, 定时器到时发送 `SIGALRM` 信号.
- `ITIMER_VIRTUAL`: 以该进程在用户态下运行的时间(进程虚拟时间)计算, 定时器到时发送 `SIGVTALRM` 信号.
- `ITIMER_PROF`: 以该进程在用户态和内核态下运行的时间计算, 定时器到时发送 `SIGPROF` 信号.

上述所有定时器信号的默认处理方式都是终止进程, 也可以自定义信号处理函数来处理这些信号.

`new_value` 指针指向一个 `itimerval` 结构体, 用于设置定时器的时间间隔和初始值. 而 `old_value` 所指向
的 `itimerval` 结构体则用于保存之前的定时器设置. 如果我们不关心之前的定时器设置, 可以将 `old_value`
参数传入 `NULL`.

`itimerval` 结构体定义如下:

```c
struct itimerval {
    struct timeval it_interval;  // 定时器的间隔时间
    struct timeval it_value;     // 定时器的初始值
};

struct timeval {
    time_t tv_sec;  // 秒
    suseconds_t tv_usec;  // 微秒
};
```

如果 `new_value` 的 `it_interval` 值全为 0, 则定时器只会在 `it_value` 时间到达时触发一次. 如果
`new_value` 的 `it_interval` 与 `it_value` 值都为 0, 则定时器会被关闭. 否则, 定时器会在 `it_value`
时间到达时触发一次, 然后每隔 `it_interval` 时间再次触发.

每个进程只能分别设置一个 `ITIMER_REAL`, `ITIMER_VIRTUAL`, `ITIMER_PROF` 类型的定时器. 如果进程第二次
调用 `setitimer()` 设置同一类型的定时器, 则会覆盖之前的设置.

### 应用场景

1. 在进行阻塞式 I/O 操作时, 可以设置一个 `ITIMER_REAL` 类型的定时器.
   1. 当 I/O 操作超时时, 会收到 `SIGALRM` 信号, 可以在信号处理函数中取消 I/O 操作.
   1. 当 I/O 操作正常完成时, 可以调用 `setitimer()` 关闭定时器.
      1. 这一步理论上存在 race condition, 但因为这类超时时间通常是秒级的, 因此几乎不会遇到这种情况.
   1. 这只是简单的用法, 更稳妥且完善的做法是使用 `select()` 或 `poll()` 等函数来实现 I/O 超时.
1. ...

## 休眠

低精度的休眠函数, 以秒为单位:

```c
#include <unistd.h>

unsigned int sleep(unsigned int seconds);
```

高精度的休眠函数, 以微秒为单位:

```c
#include <time.h>

int nanolsleep(const struct timespec *req, struct timespec *rem);

struct timespec {
    time_t tv_sec;  // 秒
    long tv_nsec;   // 纳秒(nanoseconds)
};
```

`nanosleep()` 函数会使调用进程休眠 `req` 参数指定的时间, 精确到纳秒. `nanosleep()` 可能会被信号中断,
如果休眠被中断, 则返回非 -1 并将 errno 的值设为 EINT. 如果 `nanosleep()` 的 `rem` 参数不为 NULL, 则
在异常中断时, 会将剩余的休眠时间保存在 `rem` 中, 方便后续继续休眠.

Linux 还提供了一个 `clock_nanosleep()` 函数:

> https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html

```c
#include <time.h>

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
```

`clock_nanosleep()` 与 `nanosleep()` 类似, 但可以指定使用的时钟类型, 以及更多的选项.

`clock_id` 参数可以取值为:

- `CLOCK_REALTIME`: 系统实时时间时钟.
- `CLOCK_TAI`: 一个系统级的时钟, 但是会计算闰秒.
- `CLOCK_MONOTONIC`: 从系统启动开始计时, 不受系统时间调整的影响.
- `CLOCK_PROCESS_CPUTIME_ID`: 本进程的 CPU 时间时钟.

`flags` 参数可以取值为:

- `0`: 默认值, 参数使用相对时间.
- `TIMER_ABSTIME`: `request` 参数指定的时间是绝对时间.

在使用默认的相对时间时, 每次休眠都可能会受到系统的影响而产生一定误差, 这个误差会随着时间不断累积. 因
此, 如果需要更精确的休眠时间, 可以使用 `TIMER_ABSTIME` 标志, 使 `request` 参数指定的时间为绝对时间,
这样就能避免相对时间的误差累积问题.

### Python 中的 sleep 实现

> https://docs.python.org/3/library/time.html#time.sleep

官方文档对 UNIX 系统下 `time.sleep()` 的实现描述如下:

- Use `clock_nanosleep()` if available (resolution: 1 nanosecond);
- Or use `nanosleep()` if available (resolution: 1 nanosecond);
- Or use `select()` (resolution: 1 microsecond).

## POSIX 时钟

> https://man7.org/linux/man-pages/man3/clock_gettime.3.html

```c
#include <time.h>

// 查询某一时钟的当前时间, clock_id 指定时钟类型, tp 用于保存时间
int clock_gettime(clockid_t clock_id, struct timespec *tp);

// 查询某一时种的解析度
int clock_getres(clockid_t clock_id, struct timespec *res);

// 设置某一时钟的时间
// 如果指定的 clock_id 时钟不支持设置时间, 则返回 -1 并将 errors 设为 EINVAL
int clock_settime(clockid_t clock_id, const struct timespec *tp);
```

其中 `clock_id` 参数用于指定时钟类型, 可以取值为:

- `CLOCK_REALTIME`: 系统实时时间时钟.
- `CLOCK_TAI`: 一个系统级的时钟, 但是会计算闰秒.
- `CLOCK_MONOTONIC`: 从系统启动开始计时, 不受系统时间调整的影响.
- `CLOCK_PROCESS_CPUTIME_ID`: 本进程的 CPU 时间时钟.
- `CLOCK_THREAD_CPUTIME_ID`: 本线程的 CPU 时间时钟.
- ...

## POSIX 间隔定时器

> https://man7.org/linux/man-pages/man2/timer_create.2.html

相比 UNIX `setitimer()`, POSIX 的 timers API 提供了更加灵活的定时器功能:

- 能够创建多个定时器.
- 定时器的超时通知可以通过信号或线程回调函数来实现.
- 可以选择在超时时发送哪个信号
- 可以获得一个定时器重复执行的次数(timer overrun count).

```c
#include <time.h>

// 创建一个新的定时器
// clockid 指定时钟类型, sevp 用于指定定时器的通知方式, timerid 用于保存定时器的 ID
int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);

struct sigevent {
    int sigev_notify;  // 通知方式, 可以取值为 SIGEV_SIGNAL, SIGEV_THREAD 或 SIGEV_NONE
    int sigev_signo;   // 通知时发送的信号
    union sigval sigev_value;  // 通知时传递的数据
    void (*sigev_notify_function)(union sigval);  // 通知时调用的函数
    pthread_attr_t *sigev_notify_attributes;  // 通知时使用的线程属性
};

union sigval {
    int sival_int; // 传递的整数值
    void *sival_ptr;  // 传递的指针
};
```

`clockid_t` 前面已经介绍过, 这里不再赘述.

`sigev_notify` 可以取值为:

- `SIGEV_NONE`: 定时器超时时不做任何通知. 但可以通过 `timer_gettime()` 获取定时器的超时情况.
- `SIGEV_SIGNAL`: 定时器超时时发送 `sigev_signo` 指定的信号.
- `SIGEV_THREAD`: 定时器超时时调用 `sigev_notify_function` 函数. 就像它是一个线程的启动函数一样(? 没
  理解)
- `SIGEV_THREAD_ID`(Linux-specific): 与 `SIGEV_SIGNAL` 类似, 但信号会被发送到
  `sigev_notify_thread_id` 指定的线程.
  - 这个 flag 仅供线程库使用, 应用程序不应该使用这个 flag.

## timerfd API

Linux 提供了 `timerfd` API, 使得用户空间程序可以通过文件描述符来读取定时器的超时通知. 这样我们就能使
用各种基于文件描述符的 I/O 复用函数(如 `select()`, `poll()`, `epoll()` 来监控定时器了.

```c
#include <sys/timerfd.h>

// 创建一个新的定时器文件描述符
int timerfd_create(int clockid, int flags);

// 设置定时器的超时时间和间隔时间
int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
// 查询定时器的超时时间和间隔时间
int timerfd_gettime(int fd, struct itimerspec *curr_value);
```

### 多线程下的定时器

当调用 fork 时, 子进程会继承父进程的文件描述符, 因此子进程将与父进程共享同一个定时器文件描述符. 在调
用 exec 时, 新进程也会继承父进程的 timerfd 文件描述符.

### 从 timerfd 文件描述符中读取信息

可以直接使用 read 调用来读取 timerfd 文件描述符.

如果发起 read 调用前, 定时器已经超时, 则 read 会立即返回, 并返回一个 8 字节的整数表示自定时器创建以
来或者上次 read 调用后, 定时器超时的次数.

如果发起 read 调用后, 定时器还未超时, 则 read 调用会阻塞, 直到定时器超时.

因此 read 调用的目标缓冲区的大小必须至少为 8 字节.
