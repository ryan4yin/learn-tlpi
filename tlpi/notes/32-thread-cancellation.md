# 线程取消

> https://man7.org/linux/man-pages/man3/pthread_cancel.3.html

前面介绍过, 每个线程可以自行调用 `pthread_exit()` 终止自己, 或者从线程函数中返回, 以达到线程的终止目
的.

但有时我们需要在一个线程中终止另一个线程, 这时就需要用到线程取消功能.

线程取消是一种优雅的终止线程的方法, 它允许一个线程通知另一个线程, 让其终止. 但线程取消并不是强制性
的, 取消请求可以被忽略.

```c
#include <pthread.h>

// 请求取消线程
int pthread_cancel(pthread_t thread);
```

## 取消状态及类型

如前所述, 线程收到取消请求后, 具体会发生什么取决于线程的取消状态和取消类型.

```c
#include <pthread.h>

// 设定线程的取消状态
// 如果 oldstate 不为 NULL, 则将原状态存入 oldstate
int pthread_setcancelstate(int state, int *oldstate);

// 设定线程的取消类型
// 如果 oldtype 不为 NULL, 则将原类型存入 oldtype
int pthread_setcanceltype(int type, int *oldtype);

// 此函数是一个空函数, 仅用于生成取消点
// 主要用途: 在不包含任何取消点的线程中人为添加取消点, 以便线程能够响应取消请求.
//           如果线程被取消， 那么在该线程运行到 pthread_testcancel() 时会响应取消请求, 立即终止.
int pthread_testcancel(void);
```

取消状态的取值:

- PTHREAD_CANCEL_DISABLE: 线程忽略取消请求, 即该线程不可被取消.
- PTHREAD_CANCEL_ENABLE: 线程接受取消请求, 即该线程可被取消. **这也是线程的默认状态**.

取消类型的取值:

- PTHREAD_CANCEL_ASYNCHRONOUS: 线程可以在任何时刻被取消. 能安全地被异步取消的函数能做的事情非常有限,
  因此很少使用此状态.
- PTHREAD_CANCEL_DEFERRED(推迟): 线程只能在取消点被取消. 取消请求被延迟到线程到达取消点时才会生效.
  **这也是线程的默认类型**.

## 取消点

当线程的取消状态为启用, 取消类型为推迟时, 线程只有在运行到某一取消点时才会响应取消请求. 这也是所有线
程的默认行为.

在 [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html) 中搜索 `Cancelation points`,
就能找到所有的取消点函数.

## 清理函数

如果进程有 `atexit()` 等函数用于注册退出时的清理函数, 那么线程也有类似的功能.

```c
#include <pthread.h>

// 注册线程退出时的清理函数(将其添加到清理函数栈的顶端)
int pthread_cleanup_push(void (*routine)(void *), void *arg);
// 注销线程退出时的清理函数(从清理函数栈的顶端移除一个函数)
int pthread_cleanup_pop(int execute);
```





