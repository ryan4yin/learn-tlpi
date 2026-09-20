# 程序执行

## 执行新程序: execve()

24 章就有介绍过, execve() 系统调用用一个新程序替换当前进程的程序文本段，并且为新程序初始化栈段、数据
段和堆段.

最常见的用法是先 fork() 一个子进程, 然后在子进程中调用 execve() 来执行新程序.

```c
#include <unistd.h>

// 执行新程序
// filename: 新程序的文件名
// argv: 新程序的命令行参数
// envp: 新程序的环境变量, 一个 NULL 结尾的字符串(char*)数组, 每个字符串格式为 "name=value"
int execve(const char *filename, char *const argv[], char *const envp[]);
```

因为 execve() 是彻底取代了当前的进程, 所以除非发生错误, 否则它永远不会返回. 如果发生了错误, 它会返回
-1, 并且设置 errno 为相应的错误码:

- EACCESS: 没有权限访问文件
- ENOENT: 文件不存在
- ENOEXEC: 文件不是一个可执行文件
- ETXTBSY: 存在其他进程已经以写入模式打开了此文件
- E2BIG: argv 或 envp 参数列表太长

## 构建于 execve() 之上的库函数

这些函数的命名规则是 exec + 类型 + 方式:

- 类型: l 表示参数是一个以 `(char *) NULL` 结尾的 `const char *arg, ...` 变长参数列表(list), v 表示
  参数是一个字符串数组(vector)
- 方式: e 表示需要提供环境变量(environment), p 表示会在 PATH 环境变量指定的目录中查找对应的可执行文
  件

```c
#include <unistd.h>

// 与 execve 的区别在于, args 参数是以 `const char *arg, ...` 的形式传递的
int execle(const char *path, const char *arg, ... /*, (char *) NULL, char *const envp[] */);
// 与 execle 的区别在于, 没有 envp 参数
int execl(const char *path, const char *arg, ... /*, (char *) NULL */);
// 与 execl 的区别在于, argv 参数是一个字符串数组, 而非以 `const char *arg, ...` 的形式传递
int execv(const char *path, char *const argv[]);

// 只要求提供文件名, 会在 PATH 环境变量指定的目录中查找对应的可执行文件
// 与 shell 中的命令查找机制一样
int execlp(const char *file, const char *arg, ... /*, (char *) NULL */);
int execvp(const char *file, char *const argv[]);

// 使用 fd 文件描述符作为参数
int fexecve(int fd, char *const argv[], char *const envp[]);
```

使用示例:

```c
#include <unistd.h>

int main() {
    // 执行 /bin/ls -l 命令
    execl("/bin/ls", "ls", "-l", (char *) NULL);

    char *envp[] = {"PATH=/bin", "USER=me", NULL};
    execle("/bin/ls", "ls", "-l", (char *) NULL, envp);
    return 0;
}
```

## 解释器脚本

解释器，是一种能够读取并执行文本格式命令的程序.

脚本(script), 是一种包含解释器指令的文本文件. 满足如下几个条件的文本文件就是一个 UNIX 脚本:

- 具备可执行权限
- 第一行以 `#!` 开头, 指定了解释器的路径及可选的参数. 如 `#!/bin/bash`, `#!/usr/bin/env python`
  - 这里需要注意的是, 第一行中的可选参数只能有一个, 如果你尝试使用空格分隔多个参数如 `a b`, 那么整个
    字符串 `a b` 都会被当作一个参数处理
- 余下的内容是解释器的输入, 即脚本的内容

解释器脚本同样能被 `execev()` 系统调用及其衍生函数执行, 其运行逻辑与如上所述的方式一致.

## 文件描述符与 execve() 系统调用

默认情况下, execve() 系统调用创建的新进程会继承所有父进程的文件描述符. shell 的重定向操作符与管道操
作符, 都借助了这一特性实现其功能.

譬如, `ls -l > file` 命令, shell 会先使用 dup2() 函数将 0 号文件描述符分配给 file 文件, 然后再调用
execve() 系统调用执行 `ls -l` 命令. 这样, `ls -l` 命令就会继承这个文件描述符, 并将标准输出写入到
file 文件.

> 上面的描述是一个简化的过程, 实际上 shell 会对 ls 等 builtin 命令进行特殊处理. 但是对于普通的可执行
> 文件, shell 会按照上述的方式处理.

### close-on-exec 标志(FD_CLOEXEC)

那么如果我们打开的某些文件比较重要, 不希望被任何子进程继承, 该怎么办呢?

前面的章节中介绍的 close-on-exec 标志, 就专门用来解决这个问题. 如果我们在某一个文件描述符上设置了
close-on-exec 标志, 那么在调用 execve() 系统调用时, 该文件描述符会被自动关闭, 从而避免被子进程继承.


## 信号与 execve() 系统调用

execve() 系统调用会丢弃掉调用进程的文本段, 这自然也包括了所有先前注册的信号处理函数.
因此, execev() 会将当前进程的信号处理重置为 SIG_DFL, 也就是说, 之前注册的信号处理函数会被清除.

但有一个特例, 在 Linux 中, 如果 SIGCHLD 信号被设置为 SIG_IGN, 那么在调用 execve() 之后, 该信号仍旧会
被保留 SIG_IGN 忽略状态.


## system() 函数

system() 函数是一个库函数, 用于执行 shell 命令. 它会调用 fork() 创建一个子进程, 然后在子进程中调用
exec() 系列函数执行 shell 命令.

其优点主要是简便, system() 会自行处理错误与信号, 以及底层的调用细节.
而缺点则是:

- 效率低, 它会创建 shell 进程, 并且 shell 命令中的每一个命令都会额外创建一个子进程.
- 无法对 I/O 进行精细控制. 例如, 无法获取 shell 命令的标准输出, 也无法往标准输入中写入数据.





