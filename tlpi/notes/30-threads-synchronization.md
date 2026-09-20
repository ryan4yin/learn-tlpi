# 线程 - 线程同步

## 互斥量(Mutexes)

> https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html

Mutexes, 即我们经常讲的互斥锁, 是在多线程编程(或者并发编程)中, 同步锁的一种实现方式.

互斥锁能静态分配, 也可以动态分配, 这里先介绍静态分配的互斥锁.

```c
#include <pthread.h>

// 定义并初始化一个静态分配的互斥锁
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 加锁
int pthread_mutex_lock(pthread_mutex_t *mutex);
// 解锁
int pthread_mutex_unlock(pthread_mutex_t *mutex);

// 尝试加锁, 与 pthread_mutex_lock 的区别是, 如果互斥锁已经被锁定, 则立即返回, 不会阻塞
int pthread_mutex_trylock(pthread_mutex_t *mutex);
// 带超时的加锁, 如果超时则返回 ETIMEDOUT
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout);
```

- pthread_mutex_lock: 如果互斥锁处于未锁定状态, 则将其锁定并立即返回, 否则该函数将阻塞, 直到互斥锁被解锁.
- pthread_mutex_unlock: 解锁一个已经被当前线程锁定的互斥锁.
  - 注意: 一个线程只能解锁由自己锁定的互斥锁, 解锁一个未锁定的互斥锁或者由其他线程锁定的互斥锁, 都是错误行为, 这类行为的结果取决于互斥锁的具体类型.

### 动态分配的互斥锁

动态互斥锁可按需创建与销毁:

```c
#include <pthread.h>

// 动态创建并初始化一个互斥锁
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

// 销毁一个动态创建的互斥锁
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

## 互斥锁的类型

SUSv3 规定了如下几种互斥锁类型:

- PTHREAD_MUTEX_DEFAULT: 在 Linux 上其与 PTHREAD_MUTEX_NORMAL 功能相同.
- PTHREAD_MUTEX_NORMAL: 默认类型, 不具备死锁检测功能.
  - 如果线程尝试对一个已经由自己锁定的互斥锁再次加锁, 则会导致死锁.
  - 尝试解锁一个未锁定的互斥锁, 或者由其他线程锁定的互斥锁, 其行为是未定义的.(在 Linux 上, 这两个操作都会成功)
- PTHREAD_MUTEX_ERRORCHECK: 具备错误检查功能的互斥锁, 缺点是性能较差, 推荐将其用于调试.
  - 前述的三种行为(重复加锁、错误解锁)都会导致错误返回.
- PTHREAD_MUTEX_RECURSIVE: 具备递归加锁功能的互斥锁.
  - 如果一个线程对一个已经由自己锁定的互斥锁再次加锁, 则不会导致死锁, 而是将锁的计数器加一.
  - 解锁操作会将锁的计数器减一, 直到计数器为 0 时, 互斥锁才会被解锁.

## 通知状态的改变: Condition Variables

互斥锁用于防止多个线程同时访问同一共享变量. Condition Variables 则允许一个线程就某个共享变量的变化通知其他线程, 并让其他线程等待这个通知.

Condition Variables 的主要作用是, 避免线程在等待某个条件变量时, 不断地轮询检查这个条件变量, 从而浪费 CPU 资源.

Condition Variables 总是与互斥锁一起使用, 以避免竞态条件.

```c
#include <pthread.h>

// 创建并初始化一个静态条件变量
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 通知至少一个处于等待状态的线程(性能比 broadcast 更好)
int pthread_cond_signal(pthread_cond_t *cond);
// 通知所有处于等待状态的线程
int pthread_cond_broadcast(pthread_cond_t *cond);

// 等待条件变量的变化, 在收到通知之前会一直阻塞
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
```

条件变量不会保存通知, 如果发送信号时没有线程在等待, 则信号会被直接丢弃.

### 动态分配的条件变量

```c
#include <pthread.h>

// 创建并初始化一个动态条件变量
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

// 销毁一个动态条件变量
int pthread_cond_destroy(pthread_cond_t *cond);
```


