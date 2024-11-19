# 信号: 基本概念

信号是事件发生时对进程的通知机制, 有时也称之为软件中断.

信号有多种用途, 但主要还是用于内核通知进程发生了某种事件. 引发内核发送信号给进程的各类事件如下:

- 硬件发生异常
  - 如除零错误, 无效内存访问等
- 用户输出了能产生信号的终端特殊字符
  - 如中断字符 Control-C, 暂停字符 Control-Z
- 发生了软件事件
  - 例如, 针对文件描述符的 I/O 事件, 定时器到期, CPU 时间片用完, 子进程退出等

上述内核事件也称为传统信号或标准信号, 编号 1-31.

Linux 中除了标准信号外, 还有一种叫做实时信号, 后面会详细介绍.

## 信号的处理

进程收到信号后, 默认情况下会根据信号的类型和进程当前的状态执行如下操作之一:

1. 忽略信号.
   - 即将信号丢弃, 信号对进程没有任何影响.
2. 终止(杀死)进程.
3. 生成核心转储文件(core dump)并终止进程.
   - 通常是由于进程发生了严重错误, 无法继续执行.
   - coredump 文件包含了进程的内存映像, 可通过调试器(如 gdb)加载分析.
4. 停止进程.
   - 暂停进程的执行, 直到收到 SIGCONT 信号.
5. 恢复进程.
   - 从停止状态恢复进程的执行.

进程自身对信号的处理也能有一定的控制权, 可以设置如下几种处理方式:

1. 采取默认行为
2. 忽略信号
3. 执行预先定义的信号处理函数

### 使用信号的案例

1. 通过 `kill -9` 命令强制杀死进程.
2. Nginx/Promehteus 等服务通过 `SIGHUP` 信号重新加载配置.
3. `Ctrl-C` 信号终止正在运行的程序.
4. neovim 在程序崩溃时会生成 coredump 文件.
   - 这说明 neovim 在 SIGSEGV 信号的处理函数中调用了 abort() 函数, 它会产生一个 SIGABRT 信号, 进而使
     内核生成 coredump 文件.

## 信号类型与默认行为

两个无法被捕获或忽略的信号:

- SIGKILL
  - 默认行为: 终止进程
  - 无法被捕获或忽略, 用于强制终止进程. 除非进程卡在硬件相关的 I/O 操作上, 否则 SIGKILL 信号会立即终
    止进程.
- SIGSTOP
  - 默认行为: 停止进程
  - 无法被捕获或忽略, 用于暂停进程.

其他信号:

- SIGABRT
  - 默认行为: 终止进程并生成 coredump 文件.
  - 由 abort() 函数调用引发.
- SIGALRM
  - 默认行为: 无
  - 由 alarm() 或 setitimer() 函数调用引发.
- SIGBUS
  - 默认行为: 无
  - 由硬件错误引发, 如内存访问越界.
- SIGCHLD
  - 默认行为: 忽略
  - 子进程退出时向父进程发送.
- SIGCONT
  - 默认行为: 恢复进程
  - 由 SIGSTOP 或 SIGTSTP 信号后恢复进程.
- SIGFPE
  - 默认行为: 无
  - 由除零错误引发.
- SIGHUP
  - 默认行为: 无
  - 当终端断开时发送给进程. 另外此信号也常被 nginx 等守护进程用于重新加载配置, 系统管理员可手动发送
    此信号给进程以触发配置重载.
- SIGILL
  - 默认行为: 无
  - 由非法指令引发.

## 自定义信号处理器

当信号发生时, 内核会中断主进程程序, 调用进程注册的信号处理函数, 处理函数执行完毕后, 再返回到主进程程
序中继续执行. 这也是信号被称为软中断的原因.

有两个系统调用可以用于注册信号处理函数:

- `signal`
  - 早期的信号处理函数, 但不推荐使用.
- `sigaction`
  - 更加灵活的信号处理函数, 推荐使用.

这里只介绍 `sigaction` 函数.

```c
#include <signal.h>

int sigaction(int signum,
    const struct sigaction *act,  // 新的信号处理函数及其选项
    struct sigaction *oldact      // 旧的信号处理函数及其选项会被保存在这里, 如果不关心, 可传入 NULL
);

struct sigaction {
    void (*sa_handler)(int);  // 信号处理函数
    sigset_t sa_mask;         // 信号掩码, 用于阻塞其他信号
    int sa_flags;             // 信号处理函数的选项
};
```

## 信号相关的处理函数

```c
#include <signal.h>

// 发送信号, 类似 kill 命令
int kill(pid_t pid, int sig);

int raise(int sig);  // 向进程自身发送信号

// 信号集合
int sigemptyset(sigset_t *set);  // 初始化一个空的信号集合
int sigfillset(sigset_t *set);   // 初始化一个包含所有信号(标准信号+实时信号)的信号集合
int sigaddset(sigset_t *set, int signum);  // 将指定信号添加到信号集合中
int sigdelset(sigset_t *set, int signum);  // 从信号集合中删除指定信号
int sigismember(const sigset_t *set, int signum);  // 判断指定信号是否在信号集合中
int sigandset(sigset_t *dest, const sigset_t *left, const sigset_t *right);  // 将两个信号集合进行与操作
int sigorset(sigset_t *dest, const sigset_t *left, const sigset_t *right);   // 将两个信号集合进行或操作

// 阻塞信号传递(信号掩码)
// 阻塞信号后, 信号不会被立即发送给进程, 而是等待信号解除阻塞后再处理.
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

// 查询当前已经发生, 但因为被阻塞而未处理的信号
// NOTE: 注意信号集不记录信号发生的次数, 只记录信号是否发生.
int sigpending(sigset_t *set);
```

其他函数:

```c
#include <unistd.h>

// 暂停进程, 直到收到一个信号
int pause(void);
```

