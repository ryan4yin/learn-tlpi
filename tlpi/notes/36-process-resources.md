# 进程资源

Linux 进程资源管理涉及 CPU、内存、I/O 和文件描述符等资源的分配与限制，主要通过以下 API 和机制实现：

## 进程资源监控 getrusage

getrusage 系统调用返回调用进程或其子进程用掉的各类系统资源的统计信息，其原型如下：

```c
#include <sys/resource.h>
int getrusage(int who, struct rusage *usage);
```

### 参数说明

- **`who`**：指定监控对象：
  - `RUSAGE_SELF`：当前进程的资源使用。
  - `RUSAGE_CHILDREN`：所有已终止子进程的资源使用。
  - `RUSAGE_THREAD`（Linux 特有）：当前线程的资源使用。
- **`usage`**：输出参数，指向 `struct rusage` 结构体，包含资源使用详情。

### `struct rusage` 关键字段

```c
struct rusage {
    struct timeval ru_utime;  // 用户态 CPU 时间（秒 + 微秒）
    struct timeval ru_stime;  // 内核态 CPU 时间
    long   ru_maxrss;        // 最大常驻集大小（KB）
    long   ru_minflt;        // 次缺页（无需磁盘 I/O）
    long   ru_majflt;        // 主缺页（需磁盘 I/O）
    long   ru_inblock;       // 文件系统输入操作次数
    long   ru_oublock;       // 文件系统输出操作次数
    // 其他字段略...
};
```

### 应用场景

1. **性能分析**：
   - 通过 `ru_utime` 和 `ru_stime` 计算进程的 CPU 占用率。
   - 使用 `ru_maxrss` 监控内存峰值使用量。
2. **监控告警**：
   - 有些应用程序可能会提供对自身资源使用情况的监控 API，可用于对接 Prometheus 等监控告警系统。
3. **调试**：
   - `ru_majflt` 和 `ru_minflt` 可帮助识别内存瓶颈。
4. **资源限制**：
   - 结合 `setrlimit` 实现动态资源调控。

当然 Linux 中还有很多其他查询系统资源使用情况的手段，一般来讲 `/proc/<pid>/stat` `/proc/self`
等 proc 文件系统 API 才是首选，因为 `/proc` 更具实时性，查询简单、信息全面、开销也低。

在一些更高级的场景中，可能还会额外使用 eBPF 相关工具对内核进行更细致的分析。

### 示例代码

```c
#include <stdio.h>
#include <sys/resource.h>

void print_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("User CPU time: %ld.%06ld sec\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    printf("System CPU time: %ld.%06ld sec\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    printf("Max RSS: %ld KB\n", usage.ru_maxrss);
}
```

## 进程资源管理在 K8s 与 Nginx 中的应用案例

### 1. CPU 资源管理

- **API**：`nice`, `setpriority`, `sched_setaffinity`, `cgroups`
- **应用**：
  - **Kubernetes**：
    - **配置参数**：`resources.limits.cpu` 和 `resources.requests.cpu`。
    - **底层实现**：
      - 通过 `cgroups` 的 `cpu.cfs_quota_us` 和 `cpu.cfs_period_us` 限制 CPU 时间片分配。
      - 使用 `sched_setaffinity` 绑定容器进程到指定的 CPU 核心（通过 `kubelet` 的
        `--cpu-manager-policy` 配置）。
  - **Nginx**：
    - **配置参数**：`worker_processes` 和 `worker_cpu_affinity`。
    - **底层实现**：
      - `worker_cpu_affinity` 直接调用 `sched_setaffinity` 绑定工作进程到 CPU 核心。
      - 通过 `nice` 调整工作进程的优先级（默认优先级为 0）。

### 2. 内存资源管理

- **API**：`setrlimit`, `cgroups`, `/proc/<pid>/status`
- **应用**：
  - **Kubernetes**：
    - **配置参数**：`resources.limits.memory` 和 `resources.requests.memory`。
    - **底层实现**：
      - 通过 `cgroups` 的 `memory.limit_in_bytes` 限制容器内存使用。
      - 使用 `oom_killer` 机制（基于 `/proc/<pid>/oom_score`）在内存不足时终止进程。
  - **Nginx**：
    - **配置参数**：`worker_rlimit_core` 和 `worker_shm_size`。
    - **底层实现**：
      - `worker_rlimit_core` 调用 `setrlimit(RLIMIT_CORE)` 设置核心转储文件大小限制。
      - `worker_shm_size` 控制共享内存区域的大小（通过 `mmap` 实现）。
      - 通过 `/proc/<pid>/status` 监控内存使用情况。

### 3. 文件描述符管理

- **API**：`setrlimit(RLIMIT_NOFILE)`, `/proc/<pid>/fd`
- **应用**：
  - **Kubernetes**：
    - **配置参数**：`securityContext.fsGroup` 和 `securityContext.runAsUser`。
    - **底层实现**：
      - 通过 `setrlimit(RLIMIT_NOFILE)` 设置容器的文件描述符上限。
      - 使用 `/proc/<pid>/fd` 监控容器进程打开的文件描述符。
  - **Nginx**：
    - **配置参数**：`worker_rlimit_nofile`。
    - **底层实现**：
      - 直接调用 `setrlimit(RLIMIT_NOFILE)` 设置工作进程的文件描述符上限。
      - 通过 `/proc/<pid>/fd` 检查文件描述符泄漏。
