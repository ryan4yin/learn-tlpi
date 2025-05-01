# 进程优先级和调度

> https://docs.kernel.org/scheduler/index.html

## 进程优先级

Linux 默认使用的进程调度模型是「**循环时间共享**」，该模型中每个进程轮流使用 CPU 一段时间，如此循环。循环时间共享的主要优点：

- 公平性：每个进程都有机会用到 CPU
- 响应度：每个进程在使用 CPU 之前都无需等待太长时间

进程无法直接控制自己何时使用 CPU 以及使用 CPU 的时间，但是进程可以通过修改自身或其他进程的优先级——
Nice 值——来间接影响内核调度算法。

Nice 值是一个权重因素，它使内核调度算法倾向于优先调度高优先级的进程（即 Nice 值低的进程），但这不会导致 CPU 的使用权完全被高 Nice 值的进程垄断。Nice 值的范围是 -20（最高优先级）到 19（最低优先级），默认值为 0。

### 注意事项

1. **权限要求**：

   - 特权进程（CAP_SYS_NICE）能修改任意进程的优先级。
   - **非特权进程的限制**：
     - 从 Linux 2.6.12+ 开始，非特权进程可以**提高自身优先级**（即降低 Nice 数值），但仍受限于
       `RLIMIT_NICE` 资源限制。
     - 无法将 Nice 值设置为低于 `RLIMIT_NICE` 定义的硬限制（即无法超过该优先级上限）。
     - 只能修改属于同一用户的进程的优先级。

2. **实时调度策略**：对于实时进程（使用 `SCHED_FIFO` 或 `SCHED_RR` 调度策略），优先级由
   `sched_priority` 决定，而非 Nice 值。

   - **非特权进程的限制**：
     - 默认无法切换到实时调度策略（`SCHED_FIFO` 或 `SCHED_RR`），但可通过 `CAP_SYS_NICE` 或调整
       `/proc/sys/kernel/sched_rt_runtime_us` 放宽限制。
     - 通常只能使用默认的 `SCHED_OTHER` 策略。

3. **资源限制**：可通过 `getrlimit` 和 `setrlimit` 限制进程的 CPU 时间等资源。
   - **非特权进程的限制**：
     - 只能设置比当前更严格的资源限制（无法放宽限制）。
     - 无法修改其他进程的资源限制。

### 获取和修改优先级

```c
#include <sys/resource.h>

// 获取进程的优先级（Nice 值）
// pid: 目标进程 ID，0 表示当前进程
// which: 指定优先级类型，PRIO_PROCESS（进程）、PRIO_PGRP（进程组）、PRIO_USER（用户）
// 返回值：成功返回 Nice 值（-20 到 19），失败返回 -1 并设置 errno
int getpriority(int which, id_t pid);

// 设置进程的优先级（Nice 值）
// pid: 目标进程 ID，0 表示当前进程
// which: 指定优先级类型，PRIO_PROCESS（进程）、PRIO_PGRP（进程组）、PRIO_USER（用户）
// prio: 新的 Nice 值（-20 到 19）
// 返回值：成功返回 0，失败返回 -1 并设置 errno
int setpriority(int which, id_t pid, int prio);

// 获取和设置进程的资源限制
// 可用于查询或修改进程的 CPU 时间、内存等资源限制
int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
```

### RLIMIT_NICE 资源限制

`RLIMIT_NICE` 定义了非特权进程能够设置的
**Nice 值下限**（即最高优先级）。其值通过以下方式影响 Nice 值：

> **注意 RLIMIT_NICE 的变化范围跟 Nice 值是相反的，RLIMIT_NICE
> 40 对应 Nice 值 -20，1 对应 Nice 值 19**.

- **默认限制**：通常为 `40`（对应允许的最低 Nice 值为 `-20`）。
- **计算公式**：  
  允许的最低 Nice 值 = `20 - RLIMIT_NICE`  
  例如：

  - 若 `RLIMIT_NICE = 40`，则最低 Nice 值为 `20 - 40 = -20`（最高优先级）。
  - 若 `RLIMIT_NICE = 20`，则最低 Nice 值为 `20 - 20 = 0`（默认优先级）。

## **CPU 亲和力（CPU Affinity）**

CPU 亲和力是一种机制，允许进程或线程绑定到特定的 CPU 核心上运行。这种绑定可以显著提高性能，尤其是在以下场景中：

- **减少缓存失效**：绑定到同一 CPU 核心可以减少进程切换导致的缓存失效。
- **避免上下文切换开销**：减少核心间的迁移开销。
- **实时性要求**：确保关键任务独占 CPU 资源。

#### **1. 相关 API**

Linux 提供了以下系统调用和函数来管理 CPU 亲和力：

##### **`sched_setaffinity` 和 `sched_getaffinity`**

```c
#include <sched.h>

// 设置进程的 CPU 亲和力
// pid: 目标进程 ID，0 表示当前进程
// cpusetsize: cpu_set_t 结构的大小（通常为 sizeof(cpu_set_t)）
// mask: 指向 cpu_set_t 的指针，表示允许运行的 CPU 核心
// 返回值：成功返回 0，失败返回 -1 并设置 errno
int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);

// 获取进程的 CPU 亲和力
// pid: 目标进程 ID，0 表示当前进程
// cpusetsize: cpu_set_t 结构的大小
// mask: 用于存储结果的 cpu_set_t 指针
// 返回值：成功返回 0，失败返回 -1 并设置 errno
int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
```

##### **`CPU_SET` 和 `CPU_ZERO` 宏**

```c
#include <sched.h>

// 初始化 CPU 掩码（清空所有 CPU 核心）
void CPU_ZERO(cpu_set_t *set);

// 将某个 CPU 核心添加到掩码中
// cpu: CPU 核心编号（从 0 开始）
void CPU_SET(int cpu, cpu_set_t *set);

// 检查某个 CPU 核心是否在掩码中
// cpu: CPU 核心编号
// 返回值：在掩码中返回非零值，否则返回 0
int CPU_ISSET(int cpu, cpu_set_t *set);
```

#### **2. 示例代码**

以下代码演示如何将当前进程绑定到 CPU 0 和 CPU 1：

```c
#include <sched.h>
#include <stdio.h>

int main() {
    cpu_set_t mask;
    CPU_ZERO(&mask);      // 清空掩码
    CPU_SET(0, &mask);    // 绑定到 CPU 0
    CPU_SET(1, &mask);    // 绑定到 CPU 1

    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        perror("sched_setaffinity failed");
        return 1;
    }

    printf("Process is now bound to CPU 0 and CPU 1\n");
    return 0;
}
```

#### **3. 注意事项**

- **权限要求**：非特权进程只能绑定到当前进程的 CPU 亲和力掩码范围内（需 `CAP_SYS_NICE`
  能力才能修改其他进程的亲和力）。
- **动态调整**：CPU 亲和力可以在运行时动态调整，但频繁修改可能导致性能下降。
- **多线程场景**：线程的 CPU 亲和力可以通过 `pthread_setaffinity_np` 和 `pthread_getaffinity_np`
  单独设置。

#### **4. 查看 CPU 亲和力**

可以通过以下命令查看进程的 CPU 亲和力：

```bash
taskset -p <PID>
```

#### **5. 内核限制**

- **`cpuset` 控制组**：在容器或虚拟化环境中，`cpuset` 控制组可以进一步限制进程可用的 CPU 核心范围。
- **`isolcpus` 内核参数**：通过内核启动参数 `isolcpus` 可以隔离特定 CPU 核心，仅供绑定进程使用。

#### cpuset 与 sched_setaffinity

Linux 的 `cpuset` 功能和 `sched_setaffinity`
系统调用都用于控制进程的 CPU 亲和性（即进程可以在哪些 CPU 核心上运行），但它们的应用场景和实现层次不同。以下是它们的关联和区别：

---

##### 1. **`cpuset` 功能**

- **作用**：  
  `cpuset` 是 Linux 内核提供的一种机制，通过 **cgroup（控制组）**
  对一组进程的 CPU 和内存资源进行隔离和管理。它可以：
  - 限制一组进程只能在指定的 CPU 核心上运行。
  - 限制一组进程只能使用指定的内存节点（NUMA 架构下尤其有用）。
- **实现方式**：
  - 通过挂载 `cgroup` 文件系统（通常是 `/sys/fs/cgroup/cpuset/`）并创建子 cgroup 来配置。
  - 通过写入 `cpuset.cpus` 和 `cpuset.mems` 文件来指定允许的 CPU 和内存节点。
  - 进程被添加到 cgroup 后，会自动继承其 CPU 亲和性设置。
- **应用场景**：
  - 适用于容器化环境（如 Docker、Kubernetes）或需要批量管理多进程 CPU 绑定的场景。
  - 支持动态调整，且对进程透明（进程无需主动调用 API）。

---

##### 2. **`sched_setaffinity` 系统调用**

- **作用**：  
  该系统调用（及其配套的 `sched_getaffinity`）允许**单个进程**显式地设置自己的 CPU 亲和性掩码（即可以运行在哪些 CPU 核心上）。
- **实现方式**：
  - 通过 `sched_setaffinity(pid_t pid, unsigned int len, cpu_set_t *mask)`
    直接修改指定进程的亲和性。
  - 需要进程主动调用或由外部工具（如 `taskset` 命令）调用。
- **应用场景**：
  - 适用于精细控制单个进程的 CPU 绑定（例如高性能计算或实时任务）。
  - 需要进程或管理员显式配置。

---

##### 3. **关联与优先级**

- **关联**：
  - 两者最终都通过内核的调度器限制进程的 CPU 亲和性。
  - 如果同时使用 `cpuset` 和 `sched_setaffinity`，**`cpuset` 的配置会覆盖 `sched_setaffinity`**。  
    （因为 `cpuset` 是 cgroup 的强制约束，而 `sched_setaffinity` 是进程级的请求。）
- **区别**：  
  | 特性 | `cpuset` (cgroup) | `sched_setaffinity` |
  |---------------------|----------------------------|-----------------------------|
  | 作用范围 | 一组进程（cgroup） | 单个进程 | | 配置方式 | 文件系统（`/sys/fs/cgroup`）| 系统调用或
  `taskset` 命令 | | 动态调整 | 支持（修改文件即时生效） | 支持 |
  | 优先级 | 更高（会覆盖亲和性设置） | 较低 | | NUMA 内存亲和性 | 支持（通过
  `cpuset.mems`） | 仅控制 CPU，不涉及内存 |

---

##### 4. **实际应用示例**

- **通过 `cpuset` 限制一组进程**：

  ```bash
  # 创建 cpuset cgroup
  mkdir /sys/fs/cgroup/cpuset/my_group
  echo "0-3" > /sys/fs/cgroup/cpuset/my_group/cpuset.cpus
  echo 0 > /sys/fs/cgroup/cpuset/my_group/cpuset.mems
  # 将进程添加到 cgroup
  echo <pid> > /sys/fs/cgroup/cpuset/my_group/tasks
  ```

  此时，进程只能运行在 CPU 0-3 上，即使它调用了 `sched_setaffinity` 尝试绑定到其他 CPU。

- **通过 `sched_setaffinity` 绑定单个进程**：
  ```c
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(1, &mask);
  sched_setaffinity(0, sizeof(mask), &mask);  // 绑定当前进程到 CPU 1
  ```
  如果该进程属于某个 `cpuset` cgroup，则实际可用的 CPU 是 `sched_setaffinity` 掩码与 `cpuset.cpus`
  的交集。

---

##### 5. **总结**

- **`cpuset`** 是 cgroup 层面的资源隔离机制，适合批量管理进程的 CPU 和内存资源。
- **`sched_setaffinity`** 是进程级的 API，适合精细控制单个进程的 CPU 绑定。
- 当两者冲突时，`cpuset` 的约束优先级更高。  
  （因为 cgroup 的设计目标是资源隔离，而 `sched_setaffinity` 是进程的主动请求。）

## 现代 Linux 调度器

Linux 6.6 引入了 EEVDF
Scheduler 作为一个新的调度器选项，该调度策略旨在逐渐取代旧的 CFS（完全公平调度）.

官方文档：

https://docs.kernel.org/scheduler/sched-eevdf.html

## Linux 可拓展调度器

sched_ext 是一个 Linux 内核中的调度器，它的行为可通过 BPF 程序去定义，从而实现相当灵活的调度策略实验与切换。

官方文档：

- https://sched-ext.com/docs/OVERVIEW
- https://docs.kernel.org/scheduler/sched-ext.html

官方仓库：

- https://github.com/sched-ext/scx
