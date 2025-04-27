# Daemon

## Linux 系统中 Daemon 进程的特征

1. **无控制终端**：Daemon 进程在后台运行，不与任何终端会话关联。
2. **脱离父进程**：通常由系统（如 `init` 或 `systemd`）启动，独立于用户会话运行。
3. **独立的会话和进程组**：Daemon 会创建自己的会话和进程组，避免受终端信号影响。
4. **以 root 或专用用户运行**：许多 Daemon 以 `root` 或专用系统用户运行，以确保安全和资源隔离。
5. **日志记录**：Daemon 通常将输出记录到文件（如
   `/var/log`）或系统日志（syslog），而非标准输出/错误。

## 创建 Daemon 进程的方式

### 1. 从 Shell 终端启动 Daemon（以 Nginx 为例）

Nginx 是一个典型的 Daemon 进程，它默认就以 daemon 方式启动，其启动流程如下：

1. **启动命令**：
   ```bash
   sudo nginx
   ```
2. **实现流程**：
   - Nginx 主进程会 fork 一个子进程并立即退出，使子进程成为孤儿进程，由 `init` 接管。
   - 子进程调用 `setsid()` 创建新会话，脱离终端控制。
   - 关闭标准输入、输出和错误流，避免占用终端资源。
   - 将工作目录切换到根目录（`/`），避免占用挂载点。
   - 通过配置文件（如 `/etc/nginx/nginx.conf`）加载服务配置并开始监听端口。

### 2. 借助 systemd 启动 Daemon

`systemd`
是现代 Linux 系统的初始化系统和服务管理器，它负责启动和管理所有系统服务（包括 daemon 进程）。

systemd 支持相当多种 daemon 管理方式，既可以以前台方式运行一个 daemon，也可以以后台方式运行。

#### 1. **后台运行模式**

`systemd` 通过单元文件（`.service`
文件）定义服务的启动和管理方式。以 Nginx 后台运行为例，其单元文件通常位于
`/lib/systemd/system/nginx.service`，内容如下：

```ini
[Unit]
Description=nginx - high performance web server
Documentation=man:nginx(8)
After=network.target

[Service]
Type=forking
PIDFile=/run/nginx.pid
ExecStartPre=/usr/sbin/nginx -t
ExecStart=/usr/sbin/nginx
ExecReload=/usr/sbin/nginx -s reload
ExecStop=/usr/sbin/nginx -s quit
PrivateTmp=true
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

- **`Type=forking`**：表示服务以 daemon 方式启动，`systemd` 会监控主进程的 fork 行为。
- **`PIDFile`**：指定进程的 PID 文件路径，在后台运行模式下，`systemd` 通过此文件跟踪服务状态。
- **`ExecStartPre`**：启动前的预检查命令（如测试配置文件）。
- **`ExecStart`**：启动服务的命令。
- **`ExecReload`** 和 **`ExecStop`**：定义服务的重载和停止命令。
- **`Restart`**：配置服务失败时的自动重启策略。

##### 2. **启动流程**

1. **加载单元文件**：`systemd` 解析单元文件，确定服务的依赖关系和启动顺序。
2. **启动服务**：执行 `ExecStart` 命令，启动 Nginx 主进程。
   - 如果 `Type=forking`，`systemd` 会等待主进程 fork 并退出后，通过 `PIDFile`
     确认子进程（daemon）是否成功启动。
3. **监控与管理**：
   - `systemd` 持续监控服务状态，记录日志（通过 `journalctl` 查看）。
   - 如果服务崩溃，根据 `Restart` 策略自动重启。

##### 3. **优势**

- **进程监控**：自动重启崩溃的服务。
- **依赖管理**：通过 `After` 和 `Requires` 定义服务启动顺序。
- **资源隔离**：支持 `PrivateTmp`、`ProtectSystem` 等安全配置。
- **日志集成**：所有服务日志统一由 `journald` 管理。

#### **前台运行模式**

systemd 也支持以前台模式运行 daemon 程序，因为 systemd 自身就是 1 号进程，因此它可以直接将一个普通进程以 daemon 方式运行。

此外容器化环境中通常 Nginx 等主应用本身就是容器中的 1 号进程，由容器管理器（如 Docker 或 systemd-nspawn）直接管理进程生命周期。

该场景下的常用手段也是禁用 Nginx 的 daemon 模式，让其以前台模式运行。

对 Nginx 而言，在其配置文件中添加 `daemon off`
即可禁用 daemon 模式，此时如果仍使用 systemd 运行该服务，对应的 systemd 单元文件需调整以下参数：

```ini
[Service]
Type=simple
ExecStart=/usr/sbin/nginx
```

- **`Type=simple`**：告知 systemd 直接管理前台进程（无需 fork）。
- **移除 `PIDFile`**：systemd 直接通过进程树跟踪服务，无需该文件传递 Nginx 的进程 ID.

## Daemon 常用设计

### 1. **配置热更新**
- **信号机制**：通过 `SIGHUP` 通知 daemon 重载配置文件（如 Nginx 的 `nginx -s reload`）。
- **原子性加载**：避免服务中断，例如：
  - Nginx 会先检查配置有效性，再优雅重启 worker 进程。
  - 其他 daemon 可能采用双缓冲或临时文件切换实现无缝更新。

### 2. **日志管理**
- **输出目标**：
  - 文件：如 `/var/log/daemon.log`，需配合 `logrotate` 管理轮转。
  - 系统日志：通过 `syslog` 协议发送到 `rsyslog` 或 `journald`。
  - 标准流：前台运行时输出到 `stdout/stderr`，由容器或 systemd 捕获（如 `docker logs` 或 `journalctl`）。
- **日志分级**：支持 `DEBUG`、`INFO`、`ERROR` 等级别，便于过滤和监控。

### 3. **进程监控**
- **心跳检测**：定期向 PID 文件或 systemd 发送存活信号（如 `sd_notify`）。
- **资源限制**：通过 `setrlimit()` 限制内存、文件描述符等，防止资源泄漏。
- **看门狗**：集成 systemd 的 `WatchdogSec` 或自定义超时检测逻辑。

### 4. **安全设计**
- **权限降级**：启动后从 `root` 切换到专用用户（如 Nginx 可通过配置指定使用的用户跟组）。
- **沙盒化**：使用 `chroot`、`namespaces` 或 `seccomp` 隔离进程。
- **最小权限**：仅开放必要的文件描述符和端口。

### 5. **优雅终止**
- **信号处理**：捕获 `SIGTERM` 和 `SIGINT`，清理资源后退出。
- **超时机制**：强制终止前等待子进程完成（如 `TimeoutStopSec` in systemd）。

### 6. **性能优化**
- **事件驱动**：采用 `epoll`（Linux）或 `kqueue`（BSD）实现高并发。
- **连接池**：复用 TCP 连接或数据库会话，减少开销。
- **延迟初始化**：按需加载资源，加速启动。

### 典型实现参考
- **Nginx**：多进程模型 + 事件驱动 + 配置热加载。
- **Redis**：单线程事件循环 + 持久化策略。
- **PostgreSQL**：多进程架构 + 预写式日志（WAL）。
