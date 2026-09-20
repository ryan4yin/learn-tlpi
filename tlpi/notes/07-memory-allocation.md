# 内存分配

各种动态数据结构都需要在运行时动态分配内存, 例如链表, 树, 图等.

动态内存分配是 C 语言中最容易出错的地方, 常常会导致内存泄漏和内存溢出等问题.

这也是 Rust 语言最大的创新点, 它通过所有权系统和借用系统来自动管理内存的分配与释放, 从而避免了内存泄漏和内存溢出等问题.

## 动态内存分配

```c
#include <stdlib.h>

// 动态分配一个 size bytes 大小的内存块, 返回指向该内存块的 void* 指针. 如果分配失败, 返回 NULL.
// 它分配的内存块的内容是未初始化的.
void *malloc(size_t size);
// 与 malloc 类似, 但是分配的内存块的内容被初始化为 0.
void *calloc(size_t nmemb, size_t size);
// 重新分配一个 ptr 指向的内存块, 使其大小为 size bytes. 返回指向新内存块的指针.
// 如果 ptr 为 NULL, 则等价于 malloc(size).
// NOTE: 应该尽量避免使用 realloc, 因为它常常会分配新内存块并执行内存拷贝操作, 从而导致性能下降.
void *realloc(void *ptr, size_t size);
void free(void *ptr);
```

glibc 的 `malloc()` 底层是基于 `mmap()`、`brk()`、`sbrk()` 等系统调用实现的，具体使用哪一个则取决于申请的内存大小。
`mmap()` 主要被用于申请分配大块内存（直接按页分配内存），而 `brk()` 则适合更小的内存块分配。

## 栈内存分配

```c
#include <alloca.h>

// 动态分配一个 size bytes 大小的内存块, 返回指向该内存块的 void* 指针.
void *alloca(size_t size);
```

`alloca` 最大的特点是它分配的内存块在栈上面, 因此在函数返回时该内存块会被自动释放, 无需手动调用 `free` 函数.

如果你需要在函数内部动态分配一个小内存块, 且不需要在函数外部使用, 那么 `alloca` 是一个很好的选择.

