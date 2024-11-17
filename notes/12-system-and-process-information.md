# 系统与进程信息

UNIX 系统提供了 `/proc` 虚拟文件系统以方便用户便捷地查询与修改各类系统与进程信息.

- `/proc/PID`: 进程的各项信息
  - `/proc/PID/status`: 提供进程的汇总状态信息, 可运行 `cat /proc/1/status` 验证
  - `/proc/PID/fd`: 进程打开的所有文件描述符
  - `/prod/PID/task/TID`: 进程的所有线程的信息, 其中内容很类似 `/prod/PID` 的内容, 大部分信息也都完
    全一致, 但 State/Pid/SigPnd/SigBlk 等信息可能各进程有所不同.
- `/proc` 下的系统信息
  - `/proc/partitions` 分区信息
  - `/proc/swaps` swap 交换分区信息
  - `/proc/version` 系统版本信息, 内容类似 `uname` 系统调用的输出
  - ...



