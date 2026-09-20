# Socket 编程

主要涉及 UNIX Domain Socket 与 Internet domain
socket 两类，Internet 部分属于计算机网络范畴，有《TCP/IP 详解》、 《计算机网络——自顶向下方法》等书籍详细介绍，这本书相对解释得很浅，因此略过。

这里主要记录下 UNIX Domain Socket 相关的使用方法。

## UNIX 与 Internet domain socket 比较

UNIX Domain Socket 和 Internet Domain Socket 的主要区别如下：

| 特性     | UNIX Domain Socket               | Internet Domain Socket                 |
| -------- | -------------------------------- | -------------------------------------- |
| 通信范围 | 仅限于同一主机上的进程间通信     | 支持跨主机的进程间通信                 |
| 性能     | 更高（无需网络协议栈处理）       | 较低（涉及网络协议栈）                 |
| 安全性   | 更高（仅限本地文件系统权限控制） | 较低（需额外安全措施如加密）           |
| 地址类型 | 文件系统路径（如 `/tmp/socket`） | IP 地址和端口号（如 `127.0.0.1:8080`） |
| 适用场景 | 高性能本地进程间通信             | 跨网络通信                             |

另外这两类 Socket 在 API 层面几乎没啥区别，可以很容易地相互转换。

UNIX Domain
Socket 的一大用途就是 C/S 服务架构的服务，例如 docker/postgresql 等程序，本机客户端通常都优先采用 UNIX
Domain Socket 的方式与后台服务通信。

## Socket 相关 API

```c
#include <sys/socket.h>
#include <sys/un.h>      // For UNIX Domain Socket
#include <unistd.h>      // For close()
#include <stdio.h>       // For perror()
#include <string.h>      // For memset()

// 1. 创建 Socket
int socket(int domain, int type, int protocol);
// - domain: AF_UNIX (UNIX Domain), AF_INET (IPv4), AF_INET6 (IPv6)
// - type: SOCK_STREAM (TCP-like), SOCK_DGRAM (UDP-like)
// - protocol: 通常为 0 (自动选择)

// 2. 绑定 Socket 到地址
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// - sockfd: socket() 返回的文件描述符
// - addr: 指向 sockaddr_un (UNIX Domain) 或 sockaddr_in (IPv4) 的指针
// - addrlen: 地址结构的大小

// 3. 监听连接 (仅用于 SOCK_STREAM)
int listen(int sockfd, int backlog);
// - backlog: 等待连接队列的最大长度（已经收到 SYN 但还未 accept() 的连接会进入这个队列）
// backlog 的最大上限由 /proc/sys/net/core/somaxconn 定义
// 在高并发 TCP 边缘网关上会存在大量连接建立请求，因此通常都必须适当调高上述参数。

// 4. 接受连接 (仅用于 SOCK_STREAM)
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// - addr: 存储客户端地址信息的结构指针
// - addrlen: 客户端地址结构的大小

// 5. 连接到服务端 (仅用于 SOCK_STREAM)
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// 6. 发送数据（仅用于 SOCK_STREAM）
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
// 6. 发送数据（仅用于 SOCK_DGRAM，无连接协议，因此每次都要指定 dest_addr）
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);

// 7. 接收数据 (仅用于 SOCK_STREAM)
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
// 7. 接收数据 (仅用于 SOCK_DGRAM，无连接协议，因此每次都要指定 dest_addr)
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

// 8. 关闭 Socket
int close(int sockfd);
```

Socket 按数据处理方式分为两类：

- 流 Socket - `SOCK_STREAM`
  1. **可靠性**：数据按顺序传输，无丢失或重复。
  2. **连接导向**：通信前需建立连接（如 TCP 的三次握手）。
  3. **适用场景**：文件传输、HTTP 请求等需要可靠传输的场景。
- 数据报 Socket - `SOCK_DGRAM`
  1. **无连接**：无需建立连接，直接发送数据。
  2. **不可靠性**：数据可能丢失、重复或乱序。
  3. **适用场景**：实时性要求高但允许少量丢失的场景（如视频流、DNS 查询）。

上述描述适用于 Internet Domain Socket，如果是在 Unix Domain
Socket 上使用，因为都是本机通信，数据报 Socket 会更灵活：

- 数据不会丢失、重复或乱序
- 可以使用很大的报文段，不必担心数据包被拆分或丢失

### 互联 socket 对

在使用 Unix Domain Socket 时，我们还可以在单个进程中一次创建一对 socket，主要用于进程间通信场景（与
`pipe()` 很类似）。这种用法相对普通方法的优点是：

- 这对 socket 仅在相关进程内可使用，对其他进程不可见，安全性更好。

### 抽象 Socket 名字空间

在使用 Unix Domain Socket 时，如果将 socket 地址中的 `sun_path` 设为 NULL（第一个字节值为
`\0`），那这样创建出来的 socket 将不会在文件系统上创建任何路径名，其优点：

- 无需担心名称冲突。
- 在使用完毕后，系统会自动清理掉相关资源，无需手动删除对应的路径名。
- 因为不用创建对应的文件路径名，也就无需具备对文件系统的写权限，在权限受限时比较有用。

### UNIX Domain Socket 完整示例

以下是一个完整的 UNIX Domain Socket 示例，包含服务端和客户端代码：

#### 服务端代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/example.sock"

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[256];

    // 1. 创建 Socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. 绑定 Socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH); // 确保路径未被占用
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. 监听连接
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    // 4. 接受连接
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 5. 接收数据
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_fd, buffer, sizeof(buffer), 0) == -1) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Received: %s\n", buffer);

    // 6. 关闭 Socket
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH); // 清理 Socket 文件
    return 0;
}
```

#### 客户端代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/example.sock"

int main() {
    int sockfd;
    struct sockaddr_un server_addr;
    char *message = "Hello from client!";

    // 1. 创建 Socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. 连接到服务端
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. 发送数据
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 4. 关闭 Socket
    close(sockfd);
    return 0;
}
```

## TCP 连接的创建

### 1. 三次握手过程

TCP 通过三次握手建立可靠的双向通信连接，具体流程如下：

1. **SYN (Client → Server)**
   - 客户端发送 `SYN=1` 和随机初始序列号 `seq=x`，进入 `SYN_SENT` 状态。
   - **套接字 API 对应**：客户端调用 `connect()` 触发此步骤。
2. **SYN-ACK (Server → Client)**
   - 服务端收到 `SYN` 后，回复 `SYN=1`、`ACK=1`，确认号 `ack=x+1`，并发送自己的初始序列号
     `seq=y`，进入 `SYN_RCVD` 状态。
   - **套接字 API 对应**：只要服务端已调用 `listen()`，内核将自动处理此步骤。
3. **ACK (Client → Server)**
   - 客户端收到 `SYN-ACK` 后，发送 `ACK=1`，确认号 `ack=y+1`，连接建立。
   - **套接字 API 对应**：客户端的 `connect()` 成功返回，服务端的 `accept()` 返回新连接。

### 2. 为什么需要三次握手？

- **防止历史重复连接初始化造成的资源浪费**  
  若只有两次握手，网络延迟导致的旧 `SYN` 可能被误认为新请求，导致服务端资源被无效占用。
- **同步双方初始序列号**  
  序列号是 TCP 可靠传输的核心，三次握手确保双方序列号被正确同步。
- **避免资源竞争**  
  确保双方均具备发送和接收能力，避免单向连接问题。

### 3. 握手过程中的两个随机序列号 seq 有什么作用？

| 作用               | 说明                                                                                                               |
| ------------------ | ------------------------------------------------------------------------------------------------------------------ |
| **防历史连接干扰** | 网络可能存在延迟或重传，导致旧的 `SYN` 包（属于已关闭的连接）到达服务端。随机性可避免旧连接的 `SYN` 被误认为有效。 |
| **可靠传输基础**   | 为数据包排序、去重（丢弃重传导致的重复数据包）、确认提供基准（ACK=对方序列号+1）。                                 |
| **安全性增强**     | 防止攻击者伪造 `ACK` 完成握手。                                                                                    |
| **动态同步**       | 双方通过握手确认初始序列号，确保后续数据传输的一致性。                                                             |

### 序列号/ACK 号是否有溢出可能？

TCP 的序列号和 ACK 机制在设计和实现上已完美解决溢出问题：

| 机制              | 作用                                   | 实际影响                           |
| ----------------- | -------------------------------------- | ---------------------------------- |
| **序列号回绕**    | 32 位序列号耗尽后从 0 重新计数         | 通过模运算和 PAWS 解决，几乎无影响 |
| **ACK 溢出**      | ACK 号同样按模 `2^32` 处理             | 累积确认机制天然兼容溢出           |
| **PAWS 和时间戳** | 时间戳辅助判断数据包新旧，防止回绕冲突 | 现代内核默认启用，彻底规避问题     |

### 4. 高并发优化参数

在高并发场景下，Linux 内核参数需调整以避免握手阶段的性能瓶颈：

| 参数                    | 路径                                       | 说明                                  | 建议值              |
| ----------------------- | ------------------------------------------ | ------------------------------------- | ------------------- |
| `somaxconn`             | `/proc/sys/net/core/somaxconn`             | 定义 `listen()` 的 `backlog` 队列上限 | 调高（如 `4096`）   |
| `tcp_max_syn_backlog`   | `/proc/sys/net/ipv4/tcp_max_syn_backlog`   | 半连接队列（`SYN_RCVD` 状态）大小     | 与 `somaxconn` 一致 |
| `tcp_syncookies`        | `/proc/sys/net/ipv4/tcp_syncookies`        | 防御 SYN 洪水攻击（`1` 启用）         | 高并发时设为 `1`    |
| `tcp_abort_on_overflow` | `/proc/sys/net/ipv4/tcp_abort_on_overflow` | 全连接队列满时是否拒绝（`1` 拒绝）    | 通常设为 `0`        |

#### 优化示例：

```bash
# 临时生效
echo 4096 > /proc/sys/net/core/somaxconn
echo 4096 > /proc/sys/net/ipv4/tcp_max_syn_backlog
echo 1 > /proc/sys/net/ipv4/tcp_syncookies

# 永久生效（写入 /etc/sysctl.conf）
net.core.somaxconn = 4096
net.ipv4.tcp_max_syn_backlog = 4096
net.ipv4.tcp_syncookies = 1
```

### 5. 常见问题

- **半连接队列溢出**  
  若 `SYN_RCVD` 状态连接过多，可能因 `tcp_max_syn_backlog` 不足导致丢弃 `SYN`，可通过
  `netstat -s | grep "SYNs to LISTEN"` 监控。
- **全连接队列溢出**  
  `accept()` 速度慢时，`backlog` 队列满会导致新连接被丢弃，可通过 `ss -lnt` 查看 `Recv-Q` 状态。

## TCP 连接的终止

### **1. 四次挥手过程**

TCP 通过四次挥手（Four-Way Handshake）安全关闭连接，确保双方数据完全传输并释放资源。流程如下：

#### **步骤详解**

连接双方均可主动关闭连接，另一方则进入被动关闭连接的流程，也就是说如下流程中的 A 既可以是 Client 也可以是 Server:

| 步骤 | 方向  | 报文标志 | 关键字段  | 状态变化                             | 套接字 API 映射                  |
| ---- | ----- | -------- | --------- | ------------------------------------ | -------------------------------- |
| 1    | A → B | `FIN=1`  | `seq=u`   | A: `FIN_WAIT_1`                      | `close()` 或 `shutdown(SHUT_WR)` |
| 2    | B → A | `ACK=1`  | `ack=u+1` | B: `CLOSE_WAIT` <br> A: `FIN_WAIT_2` | 内核自动回复                     |
| 3    | B → A | `FIN=1`  | `seq=v`   | B: `LAST_ACK`                        | B 调用 `close()`                 |
| 4    | A → B | `ACK=1`  | `ack=v+1` | A: `TIME_WAIT` → 关闭 <br> B: 关闭   | 内核自动回复                     |

#### **流程图示**

```plaintext
A (Client)                          B (Server)
  |-------- FIN (seq=u) ----------->|  # A 主动关闭
  |<------- ACK (ack=u+1) ----------|  # B 确认
  |<------- FIN (seq=v) ------------|  # B 被动关闭
  |-------- ACK (ack=v+1) --------->|  # A 确认
```

---

### **2. 为什么需要四次挥手？**

- **双向连接独立关闭**：  
  TCP 是全双工协议，每个方向需独立关闭。`FIN` 表示“不再发送数据”，但可继续接收数据。
- **确保数据完整性**：  
  等待 `LAST_ACK` 和 `TIME_WAIT` 状态，防止最后的数据包或 `ACK` 丢失。

---

### **3. 关键机制与注意事项**

#### **(1) `TIME_WAIT` 状态**

- **作用**：
  - 确保最后一个 `ACK` 到达对端（若丢失，对端重传 `FIN`）。
  - 避免旧连接的延迟数据包干扰新连接（通过 2MSL 等待时间清除残留报文）。
- **时长**：  
  `2 * MSL`（Maximum Segment Lifetime，默认 60s，Linux 可调 `/proc/sys/net/ipv4/tcp_fin_timeout`）。

#### **(2) 半关闭（Half-Close）**

- **场景**：  
  一方调用 `shutdown(SHUT_WR)` 发送 `FIN`，但仍可接收数据（如 HTTP/1.1 的 `Connection: close`）。
- **API 区别**：  
  `close()` 直接关闭双向连接；`shutdown()` 支持单向关闭。

#### **(3) 异常终止**

- **`RST` 报文**：  
  强制终止连接（如端口未监听、进程崩溃），跳过正常挥手流程。  
  **触发条件**：
  - 向已关闭的 Socket 写数据。
  - `SO_LINGER` 选项设置超时为 0。

---

### **4. 常见问题与优化**

#### **(1) `TIME_WAIT` 过多**

- **现象**：高并发服务（如反向代理）可能因为主动关闭大量连接，导致 `TIME_WAIT` 连接占用大量
  `(local_ip, local_port, remote_ip, remote_port)` 组合，导致临时端口耗尽。
- **原因分析**
  1. **HTTP 协议行为**
  - 短连接模式（如 HTTP/1.0 或无 Keep-Alive）下，反向代理作为服务端需主动关闭连接。QPS 稍微高一点就很容易导致端口耗尽。
  - 即使启用 Keep-Alive，超时或达到最大请求数后仍会主动终止连接。在相关参数没调优或者并发过高的时候仍然可能导致端口耗尽。
  2. **客户端异常**
  - 客户端未正常关闭时（如崩溃），反向代理被迫主动终止连接。
  3. **Nginx 实现的缺陷**：Nginx 在更新配置时会启动新 workers 替换旧 workers，这期间会主动中断所有客户端连接。
- **监控工具**
  ```bash
  # 查看 TIME_WAIT 状态连接数
  ss -tan | grep TIME-WAIT | wc -l
  ```
- **解决方案**：
  - 启用端口复用（`SO_REUSEADDR`/`SO_REUSEPORT`）：
    ```c
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ```
  - 调整 `tcp_max_tw_buckets`（限制 `TIME_WAIT` 数量）。
  - 改用长连接（如 HTTP/1.1 Keep-Alive）。

#### **(2) 连接泄漏**

- **场景**：  
  服务端未正确处理 `CLOSE_WAIT`（未调用 `close()`），导致连接堆积。
- **排查工具**：
  ```bash
  netstat -antp | grep CLOSE_WAIT
  ss -o state close-wait
  ```

---

### **5. 常见问题扩展**

#### **半连接队列溢出**

- **现象**：`SYN_RCVD` 状态连接过多，导致 `tcp_max_syn_backlog` 不足。
- **监控**：
  ```bash
  netstat -s | grep "SYNs to LISTEN"
  ```

#### **全连接队列溢出**

- **现象**：`accept()` 速度慢时，`backlog` 队列满导致新连接被丢弃。
- **检查**：
  ```bash
  ss -lnt | grep Recv-Q
  ```

### **6. 完整代码示例**

#### **服务端主动关闭连接**

```c
// 服务端调用 close() 触发 FIN
close(client_fd);  // 进入 LAST_ACK
// 收到最后一个 ACK 后完全关闭
```

#### **客户端处理半关闭**

```c
shutdown(sockfd, SHUT_WR);  // 发送 FIN，进入 FIN_WAIT_1
// 仍可调用 recv() 读取剩余数据
while (recv(sockfd, buf, sizeof(buf), 0) > 0) {
    // 处理数据
}
close(sockfd);  // 完全关闭
```

### **内核参数调优**

| 参数                  | 路径                                     | 说明                                               | 建议值       |
| --------------------- | ---------------------------------------- | -------------------------------------------------- | ------------ |
| `tcp_fin_timeout`     | `/proc/sys/net/ipv4/tcp_fin_timeout`     | 控制 `TIME_WAIT` 状态的超时时间（秒）              | `30`         |
| `tcp_tw_reuse`        | `/proc/sys/net/ipv4/tcp_tw_reuse`        | 允许复用处于 `TIME_WAIT` 状态的连接（需内核 ≥4.1） | `1`（启用）  |
| `tcp_max_tw_buckets`  | `/proc/sys/net/ipv4/tcp_max_tw_buckets`  | 限制系统中 `TIME_WAIT` 连接的最大数量              | `200000`     |
| `ip_local_port_range` | `/proc/sys/net/ipv4/ip_local_port_range` | 定义临时端口的可用范围（格式：`min max`）          | `1024 65535` |
| `tcp_syncookies`      | `/proc/sys/net/ipv4/tcp_syncookies`      | 防御 SYN 洪水攻击（`1` 启用，高并发时建议启用）    | `1`          |
| `somaxconn`           | `/proc/sys/net/core/somaxconn`           | 定义 `listen()` 全连接队列的最大长度               | `4096`       |
| `tcp_max_syn_backlog` | `/proc/sys/net/ipv4/tcp_max_syn_backlog` | 控制半连接队列（`SYN_RCVD` 状态）的大小            | `4096`       |

---

### ** 端口重用的实现原理**

#### **(1) `SO_REUSEADDR`**

- **作用机制**：  
  **允许绑定处于 `TIME_WAIT` 状态的地址**（本地监听的 IP + 端口组合），避免服务重启时因该地址上存在
  `TIME_WAIT` 连接导致 `bind()` 失败。
  - **实现说明**：
    - **Linux 的 socket 实现默认使用了比 TCP 更严格的约束**，如果主机上有任何能匹配到本地 IP+端口 的 TCP 连接，则
      `bind()` 调用会失败。此参数放宽了这一限制，使得此类 `bind()`
      调用能够成功，这更接近 TCP 的需求（TCP 只要求源地址+目标地址四元组不能冲突）。
- **使用场景**

  - **服务器快速重启**：  
    大多数服务需显式启用 `SO_REUSEADDR` 以实现平滑重启。

- **为什么能这么做**：  
  如前所述，`SO_RESUEADDR` 只是使 `bind()`
  调用不至于失败，使服务重启后能立即监听对应的端口，从而能立即为绝大部分与本机不存在 `TIME_WAIT`
  连接的客户端提供访问。但 **新建连接的四元组若与 `TIME_WAIT` 连接冲突，仍会被内核拒绝**（除非启用
  `tcp_tw_reuse`）。它完全符合 TCP 协议设计，只是放宽了 Linux 系统自身的一项检查而已。

#### **(2) `SO_REUSEPORT`**

- **作用机制**：  
  允许多个套接字（通常属于不同进程）绑定到完全相同的 IP 和端口组合，内核会自动将连接请求负载均衡到这些套接字。
  - **关键条件**：所有套接字必须显式启用 `SO_REUSEPORT`。
  - **实现细节**：
    - 内核通过哈希算法将新连接分配到不同的监听套接字（避免锁竞争）。
    - 每个套接字独立处理连接，适用于多核 CPU 的扩展。
- **使用场景**

  - **多进程负载均衡**：  
    Nginx 或 Envoy 等多进程服务，每个 Worker 进程独立监听同一端口。
  - **高性能服务器**：  
    避免单一监听队列的锁竞争，提升并发能力（如 Cloudflare 的优化案例）。

- **为什么能这么做**：  
  现代多核系统需要避免单一监听套接字的锁竞争问题。`SO_REUSEPORT` 将负载分散到多个套接字，提升并发性能。


#### **`tcp_tw_reuse`（内核参数）**
#### **作用**
- **复用 `TIME_WAIT` 连接的端口**，允许新连接直接重用 `TIME_WAIT` 状态的五元组（四元组 + 协议）。
- **修改 TCP 协议行为**：通过时间戳机制（`TCP_TIMESTAMP`）确保旧连接的延迟数据包不会干扰新连接。

#### **触发条件**
- 需同时满足：
  1. 内核启用 `tcp_tw_reuse`（默认关闭）：
     ```bash
     echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
     ```
  2. 新连接启用 `TCP_TIMESTAMP` 选项（现代内核默认开启）。
  3. 新连接的 **时间戳 > 旧连接的最后时间戳**（防止旧数据包干扰）。

#### **使用场景**
- **高并发短连接服务**：如反向代理（Nginx）、负载均衡器，减少 `TIME_WAIT` 导致的临时端口耗尽。
- **客户端优化**：客户端频繁连接同一服务端时复用端口。

---

### **6. 总结**

| 核心点             | 说明                                           |
| ------------------ | ---------------------------------------------- |
| **四次挥手必要性** | 全双工特性要求双向独立关闭                     |
| **`TIME_WAIT`**    | 防丢包和旧数据干扰，需结合 `SO_REUSEADDR` 优化 |
| **半关闭**         | `shutdown()` 提供更灵活的控制                  |
| **高并发调优**     | 复用端口、调整内核参数、监控 `CLOSE_WAIT`      |
