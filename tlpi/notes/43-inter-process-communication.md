# 进程间通信 IPC

> System V 相关的 API 已经不推荐使用，属于遗留 API，这里不多介绍。

Linux 内核提供了一系列用于进程间通信的功能，主要分为三大类：

- 通信：
  - 数据传输类：
    - 字节流：管道 Pipe、FIFO、流 socket
    - 伪终端
    - 消息队列：POSIX 消息队列、数据报 socket
  - 共享内存
    - POSIX 共享内存
    - 内存映射 mmap
- 信号（第 20 章有介绍）
  - 标准信号
  - 实时信号
- 同步
  - POSIX 信号量（编程语言中互斥锁的实现可能会用到此 API）
  - 文件锁
  - 互斥（线程）
  - 条件变量（线程）

### **IPC 工具之间的比较**

| **工具** | **适用场景**       | **性能** | **复杂度** | **跨语言支持** |
| -------- | ------------------ | -------- | ---------- | -------------- |
| 管道     | 父子进程简单通信   | 中       | 低         | 有限           |
| FIFO     | 无亲缘关系进程通信 | 中       | 中         | 有限           |
| 消息队列 | 结构化消息传递     | 中       | 高         | 良好           |
| 共享内存 | 高频数据共享       | 高       | 高         | 良好           |
| 信号量   | 同步控制           | 高       | 中         | 良好           |
| Socket   | 跨主机通信         | 低       | 高         | 优秀           |

---

### **IPC 工具在编程语言底层实现中的应用**

#### **1. Java**

- **共享内存**：`MappedByteBuffer` 类基于 `mmap` 实现。
- **信号量**：`Semaphore` 类封装 POSIX 信号量。
- **Socket**：`java.net` 包提供跨平台 Socket 支持。

#### **2. Go**

- **管道**：`chan` 类型底层可能使用匿名管道或共享内存。
- **同步**：`sync.Mutex` 和 `sync.Cond` 基于 Futex（用户态互斥）或信号量。

#### **3. Python**

- **消息队列**：`multiprocessing.Queue` 封装 POSIX 消息队列。
- **共享内存**：`multiprocessing.shared_memory` 模块基于 `shm_open`。

### **通信工具**

#### **1. 数据传输类**

##### **管道（Pipe）**

- **特点**：
  - 半双工通信（单向）。
  - 仅用于父子进程或兄弟进程之间的通信。
  - 数据以字节流形式传输。
- **API**：
  ```c
  int pipe(int pipefd[2]); // pipefd[0] 为读端，pipefd[1] 为写端
  ```
- **示例**：
  ```c
  int fd[2];
  pipe(fd);
  if (fork() == 0) { // 子进程
      close(fd[0]); // 关闭读端
      write(fd[1], "Hello", 6);
  } else { // 父进程
      close(fd[1]); // 关闭写端
      char buf[6];
      read(fd[0], buf, 6);
  }
  ```

##### **FIFO（命名管道）**

FIFO 与管道功能基本一致，主要区别就是它具备文件路径，而且其读写方式与普通文件也别无二致。

- **特点**：

  - 全系统可见的管道，通过文件系统路径标识。
  - **可用于无亲缘关系的进程间通信**。

- **API**：
  ```c
  int mkfifo(const char *pathname, mode_t mode);
  ```

也可以直接使用命令行工具创建一个 FIFO，然后在程序中使用：

```bash
mkfifo [-m mode] pathname
```

使用 mkfifo 与 tee 可以创建一个双重管线，使两个程序能同步读取上一个程序的输出：

```bash
mkfifo myfifo
# 后台运行一个程序读 fifo 管道
wc -l < myfifo &
# 使用 tee 复制一份数据输出到 fifo 管道，实现双重管线
ls -l | tee myfifo | sort -k5n
```

###### FIFO 与非阻塞 IO

在以 O_RDONLY 模式打开管道时，该系统调用将会阻塞，直到另一个进程以 O_WRONLY 模式打开 FIFO.

可通过在调用 open() 时指定 `O_NONBLOCK` 标记来实现非阻塞 I/O. 其具体效果：

| open() 模式             | 行为描述                                                |
| ----------------------- | ------------------------------------------------------- |
| `O_RDONLY`              | 阻塞，直到有进程以 `O_WRONLY` 模式打开 FIFO。           |
| `O_RDONLY + O_NONBLOCK` | 立即成功返回，即使写入端还未打开。                      |
| `O_WRONLY`              | 阻塞，直到有进程以 `O_RDONLY` 模式打开 FIFO。           |
| `O_WRONLY + O_NONBLOCK` | 如果没有读取端，立即失败并返回 `ENXIO` 错误；否则成功。 |

有时候我们可能会需要在打开 FIFO 时使用非阻塞 I/O 以便立即返回，但是在读写时却希望能使用阻塞式 I/O，或者相反。这时就需要用到
`fcntl()` 来动态修改 FIFO 文件描述符的 O_NONBLOCK 状态，其修改方式与普通文件别无二致。

##### **消息队列（POSIX）**

- **特点**：
  - 消息边界清晰，支持优先级。
  - 通过 `mq_open`、`mq_send`、`mq_receive` 等函数操作。
- **示例**：
  ```c
  mqd_t mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0666, NULL);
  mq_send(mq, "Hello", 6, 0);
  ```

##### **共享内存（POSIX）**

- **特点**：
  - 最高效的 IPC 方式，直接映射到进程地址空间。
  - 需要同步机制（如信号量）配合。
- **API**：
  ```c
  int shm_open(const char *name, int oflag, mode_t mode);
  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  ```

---

### **同步工具**

#### **1. POSIX 信号量**

- **特点**：
  - 用于进程或线程间的同步。
  - 分为命名信号量（跨进程）和匿名信号量（线程间）。
- **API**：
  ```c
  sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
  sem_wait(sem_t *sem); // P 操作
  sem_post(sem_t *sem); // V 操作
  ```

#### **2. 文件锁**

- **特点**：
  - 通过 `fcntl` 实现，支持读写锁。
  - 适用于多进程对同一文件的访问控制。
- **API**：
  ```c
  struct flock lock;
  lock.l_type = F_WRLCK; // 写锁
  fcntl(fd, F_SETLKW, &lock); // 阻塞获取锁
  ```

#### **3. 条件变量与互斥锁（线程间）**

- **特点**：
  - 通常用于线程同步，但也可通过共享内存扩展至进程间。
  - 需配合互斥锁使用。
- **API**：
  ```c
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_cond_wait(&cond, &mutex); // 等待条件
  pthread_cond_signal(&cond); // 通知条件
  ```

---
