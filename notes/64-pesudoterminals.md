# 伪终端

现代化操作系统基本都是图形化的，传统的物理终端已经销声匿迹。但是终端仍然以「伪终端」的形式在计算机系统中被广泛应用，它借助图形化窗口来模拟传统终端的功能，从而提供类似的使用体验。

伪终端是一个虚拟设备，它提供了一个 IPC 通道，通道的一端是一个期望连接到终端设备的程序，另一端则通常是一个图形化的终端模拟器程序，一个 SSH 服务器，或者一个 tmux/zellij 之类的终端多路复用程序。


伪终端的从设备表现得就像一个标准终端一样，支持所有标准终端支持的 API. 对于「设定终端线速」或「奇偶校验」等在伪终端中没有意义的 API，伪终端也支持，但是这些 API 不会产生任何效果。


## **伪终端核心 API 定义**

```c
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pty.h>
#include <termios.h>

// 1. 创建伪终端主设备
int posix_openpt(int flags);
// 参数:
//   - flags: 文件打开标志 (必选 O_RDWR，可选 O_NOCTTY 等)
// 返回值: 主设备文件描述符 (成功) 或 -1 (失败)

// 2. 设置从设备权限
int grantpt(int master_fd);
// 参数:
//   - master_fd: 主设备文件描述符
// 返回值: 0 (成功) 或 -1 (失败)

// 3. 解锁从设备
int unlockpt(int master_fd);
// 参数:
//   - master_fd: 主设备文件描述符
// 返回值: 0 (成功) 或 -1 (失败)

// 4. 获取从设备路径名
char *ptsname(int master_fd);
// 参数:
//   - master_fd: 主设备文件描述符
// 返回值: 从设备路径字符串 (如 "/dev/pts/1") 或 NULL (失败)

// 5. 打开从设备
int open(const char *pathname, int flags);
// 参数:
//   - pathname: 从设备路径 (来自 ptsname())
//   - flags: 文件打开标志 (如 O_RDWR)
// 返回值: 从设备文件描述符 (成功) 或 -1 (失败)

// 6. 终端属性控制
int tcgetattr(int fd, struct termios *termios_p);
//        int tcsetattr(int fd, int actions, const struct termios *termios_p);
// 参数:
//   - fd: 终端文件描述符
//   - termios_p: 终端属性结构体指针
//   - actions: 生效时机 (TCSANOW/TCSADRAIN/TCSAFLUSH)
// 返回值: 0 (成功) 或 -1 (失败)

// 7. 窗口大小控制
int ioctl(int fd, unsigned long request, ...);
// 常用请求:
//   - TIOCGWINSZ: 获取窗口大小 (struct winsize *)
//   - TIOCSWINSZ: 设置窗口大小 (const struct winsize *)
// 参数:
//   - fd: 终端文件描述符
//   - request: 控制命令 (如 TIOCSWINSZ)
//   - ...: 可变参数 (如 struct winsize *)
// 返回值: 0 (成功) 或 -1 (失败)

// 8. 关闭设备
int close(int fd);
// 参数:
//   - fd: 文件描述符
// 返回值: 0 (成功) 或 -1 (失败)
```

---

### **关键说明**
1. **标准性**：  
   - `posix_openpt()`, `grantpt()`, `unlockpt()`, `ptsname()` 是 POSIX 标准函数。  
   - `ioctl()` 和终端控制函数 (`tcgetattr`/`tcsetattr`) 是 Unix 通用接口。  

2. **参数约束**：  
   - `flags` 在 `posix_openpt()` 中必须包含 `O_RDWR`。  
   - `termios` 结构体用于配置终端行为（如回显、规范模式等）。  

3. **错误处理**：  
   所有函数失败时返回 `-1` 并设置 `errno`，需用 `perror()` 或 `strerror(errno)` 检查。  

4. **扩展 API**：  
   - `forkpty()`（非 POSIX，但广泛支持）：合并 `fork()` + PTY 创建流程。  
   - `login_tty()`（某些系统）：设置子进程的控制终端。  

## 完整示例

以下是一个完整的伪终端使用示例，创建一个子进程并将其输入/输出重定向到伪终端：

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pty.h>
#include <termios.h>

int main() {
    int master_fd;
    char *slave_name;
    pid_t pid;

    // 创建伪终端
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1) {
        perror("posix_openpt");
        exit(EXIT_FAILURE);
    }

    if (grantpt(master_fd) == -1) {
        perror("grantpt");
        exit(EXIT_FAILURE);
    }

    if (unlockpt(master_fd) == -1) {
        perror("unlockpt");
        exit(EXIT_FAILURE);
    }

    slave_name = ptsname(master_fd);
    if (!slave_name) {
        perror("ptsname");
        exit(EXIT_FAILURE);
    }

    printf("Slave device: %s\n", slave_name);

    // 创建子进程
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // 子进程
        close(master_fd);

        // 打开从设备并设置为控制终端
        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd == -1) {
            perror("open slave");
            exit(EXIT_FAILURE);
        }

        // 设置终端属性
        struct termios tty;
        tcgetattr(slave_fd, &tty);
        tty.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(slave_fd, TCSANOW, &tty);

        // 重定向标准输入/输出/错误到从设备
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        // 执行 shell
        execlp("bash", "bash", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // 父进程
        close(slave_fd);

        // 主设备读写逻辑
        char buf[1024];
        ssize_t n;
        while ((n = read(master_fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
        }

        close(master_fd);
    }

    return 0;
}
```

