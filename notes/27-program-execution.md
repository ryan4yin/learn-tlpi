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

- 类型: l 表示参数是一个以 `(char *) NULL` 结尾的 `const char *arg, ...` 变长参数列表(list), v 表示参
  数是一个字符串数组(vector)
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

