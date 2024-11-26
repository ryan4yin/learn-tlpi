# 线程 - 更多细节

## 线程和信号

UNIX 信号模型是基于 UNIX 进程模型而设计的, 问世比 Pthreads 要早几十年.
自然而然, 信号与线程模型之间存在一些明显的冲突.

信号与线程之间的差异意味着, 将二者结合使用将会非常复杂.
因此在设计多线程程序时, 应该尽量避免使用信号.
