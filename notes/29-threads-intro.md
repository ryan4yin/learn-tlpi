# 线程 - 介绍

在 Linux 内核中, 线程就是一种轻量的进程, 其创建速度通常比进程快 10 倍以上.

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
- 资源限制
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
