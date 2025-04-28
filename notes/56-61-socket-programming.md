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
