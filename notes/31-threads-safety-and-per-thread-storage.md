# 线程 - Thread Safety and Per-Thread Storage

## 什么是可重入 - What is Reentrancy

> https://en.wikipedia.org/wiki/Reentrancy_(computing)

可重入 Reentrancy - 即表示可以重复进入.

具体定义: 可重入是指一个函数或子程序可以在执行过程中被中断, 然后再次重新开始执行.

由此定义可以推导出:

> **可重入函数是无状态的, 即不依赖于任何全局变量或静态变量**.

因为如果依赖于全局变量或静态变量, 那么函数被中断并重新开始执行时, 全局变量或静态变量的状态可能已经发
生了变化, 从而导致函数的行为与预期不符.

## 线程安全 - Thread Safety

如果一个函数可以安全地在多个线程中并发调用, 那么我们称这个函数是线程安全的.

### 可重入与线程安全的区别

- **可重入函数是线程安全函数的真子集: 可重入函数一定是线程安全的, 但线程安全的函数不一定是可重入
  的**.
- 可重入函数是无状态的, 而线程安全函数可以有状态.

一个线程安全但不可重入的函数例子: 使用了互斥锁的函数.

如果一个函数在对一把默认类型的静态互斥锁加锁后被中断，并且重新开始执行, 那么这个函数就会触发死锁. 这
与该函数正常执行的行为不一致.

## 一些函数的可重入版本

前面提到, 「可重入函数是无状态的, 即不依赖于任何全局变量或静态变量」.

但有些函数的定义本身就依赖了某些全局变或静态变量, 这导致它们不可重入, 同时也不是线程安全的. 虽然也可
以通过为它们加互斥锁的方式使其线程安全, 但这样做性能会受到影响, 代码也会变得复杂.

SUSv3 为此定义了一些可重入版本的函数, 以便在多线程环境下使用. 其诀窍就是由调用者提供一个缓冲区, 用于
存储函数的返回值, 这样就能避免使用全局变量或静态变量.

这些函数的命名规则是在原函数名后加 `_r`, 例如 `gethostbyname_r`, `rand_r` 等.

## Per-Thread Storage

实现线程安全的最佳方法就是使其可重入, 但有时这并不容易实现, 或者改造成本太高.

作为一种折衷方案, 我们可以使用如下两种技术:

- 线程特定数据(TSD, Thread-Specific Data).
- 线程局部存储

上述两种技术的本质功能都是一样的: 使得每个线程都维护一份变量的副本, 从而隔离了线程间的状态, 使得函数
可以在多线程环境下安全地调用.

区别在于, 线程特定数据是 Pthreads API 中提供的几个函数, 使用起来更复杂些. 而线程局部存储则是通过
Linux 内核、Pthreads 实现(NPTL) 以及 C 编译器联合提供的一种机制, 只需要为变量添加 `__thread` 修饰符
即可, 用起来要简单得多.


将某一变量声明为线程局部存储的方法是在变量声明前加上 `__thread` 修饰符, 例如:

```c
// 声明一个线程局部存储的整型全局变量
__thread int tls_var;

// 声明一个线程局部存储的静态变量
static __thread int buf[1024];
```

而如果改用线程特定数据实现此功能, 则要麻烦许多:

```c
#include <pthread.h>

// 初始化一个线程特定数据键(注意此函数只应被调用一次)
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));

// 为线程特定数据键设置一个新的值(这个值应当是线程的局部变量)
int pthread_setspecific(pthread_key_t key, const void *value);

// 获取线程特定数据键的值
void *pthread_getspecific(pthread_key_t key);
```

线程特定数据本质上就是一个键值对(dict/map):

- 在每个线程中, 通过同一个键设置的值是相互独立的, 互不影响.
- 在每个线程中, 通过同一个键获取到的也是该线程自己设置的值.


