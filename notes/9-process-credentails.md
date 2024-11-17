# 进程凭证

每个进程都有一系列与之关联的用户 ID(UID) 与组 ID(GID), 有时也将这些 ID 称为进程凭证, 具体如下所示:

- **real** user ID and group ID;
- **effective** user ID and group ID;
- **saved** set-user-ID and saved set-group-ID;
- **file-system** user ID and group ID (Linux-specific);
- **supplementary group IDs**.

## real user ID 与 gorup ID

这个应该是最容易理解的, 就是进程被运行时的用户 ID 与组 ID.

- systemd service 可以通过 User 与 Group 参数设置进程的 real user ID 与 real group ID, 这两个 ID 启
  动后就无法修改了.
- 新进程会从父进程继承这些 ID.
  - 譬如在 Shell 中运行的程序会继承 Shell 的 real user ID, systemd service 默认会使用 root 账户运行.

## effective user ID 与 group ID

出于现实使用的需要, 进程的权限不是简单地直接等同于它的 real user ID 与 real group ID. 一个最常见的例
子是 `sudo` 与 `su` 命令, 用户能使用这两个命令获得以 root 或其他用户执行命令的权限.

为了实现这一功能, 一个进程的权限实际由如下三个属性确定:

- effective user ID
- effective group ID
- supplementary group IDs

简单的说, effective user ID 与 effective group ID 才是进程在当前时间点真正使用的 UID 与 GID, 用户可
以在一定条件下动态修改这两个 ID 的值, 从而在不同的权限约束下执行任务.

## Set-User-ID and Set-Group-ID 程序

每个可执行文件都具有两个特别的权限位: **set-user-ID** 位和 **set-group-ID** 位.

被设置了 set-user-ID 权限位的程序被称为 Set-User-ID 程序, Set-Group-ID 程序也是同理.

这两个权限位的特点是:

- Set-User-ID 程序会在启动时将自身进程的 effective user ID 设置成该程序二进制文件的 owner user ID
- 同理, Set-User-ID 程序也会在启动时将自身的 effective user ID 设置为该程序二进制文件的 owner group
  ID

`sudo` `su` 等系统程序就是典型的 Set-User-ID 程序, 普通用户也能借助这些命令临时伪装成其他用户来执行
任务.

> 为了安全性, 这类程序通常会借助 PAM 模块认证用户身份后才会继续工作. sudo 因为相当常用, 还出现了更现
> 代的安全优化版 - sudo-rs

我们检查下 NixOS 下这些系统程序的权限位:

```bash
# ls -al /run/wrappers/bin/
total 884
drwxr-xr-x 2 root root         300 11月12日 16:07 .
drwxr-xr-x 3 root root          80 11月12日 16:07 ..
-r-sr-x--- 1 root messagebus 66936 11月12日 16:07 dbus-daemon-launch-helper
-r-s--x--x 1 root root       66936 11月12日 16:07 fusermount
-r-s--x--x 1 root root       66936 11月12日 16:07 fusermount3
-r-s--x--x 1 root root       66936 11月12日 16:07 mount
-r-s--x--x 1 root root       66936 11月12日 16:07 newgidmap
-r-s--x--x 1 root root       66936 11月12日 16:07 newgrp
-r-s--x--x 1 root root       66936 11月12日 16:07 newuidmap
-r-s--x--x 1 root root       66936 11月12日 16:07 sg
-r-s--x--x 1 root root       66936 11月12日 16:07 su
-r-s--x--x 1 root root       66936 11月12日 16:07 sudo
-r-s--x--x 1 root root       66936 11月12日 16:07 sudoedit
-r-s--x--x 1 root root       66936 11月12日 16:07 umount
-r-s--x--x 1 root root       66936 11月12日 16:07 unix_chkpwd
```

能看到其中的可执行位 `w` 被显示成了 `s`, 这就表示它们被启用了 Set-User-ID 权限位.

可以通过 chmod 修改程序的 set-user-ID 与 set-group-ID 权限位:

```bash
# 启用 set-user-ID 权限位
chmod u+s xxx
# 关闭该权限位
chmod u-s xxx

# 启用 set-group-ID 权限位
chmod g+s xxx
chmod g-s xxx
```

## saved user ID 与 group ID

前述的 Set-User-ID 与 Set-Group-ID 权限位只会在程序启动时生效, 进程启动后它们就不再具有任何作用.

> 我想这样设计的理由是: 运行中的进程不应该依赖其二进制文件的属性, 这能避免很多问题. 我们知道即使程序
> 被删除, 运行中的进程也是不受影响的.

但确实有些程序会有在进程运行期间动态切换自身 effective user ID 与 group ID 的需求, 这是基于这样一种
最佳安全实践:

> **应以最小的权限执行各类任务, 仅在必要的时候才提升权限执行操作.**

为了实现这一功能, UNIX 专门设计了 saved user ID 与 gorup ID 这一组 ID, 它的工作方式如下:

- 在 Set-User-ID/Set-Group-ID 程序启动时, 会将进程的 effective user/group ID 设置为该程序二进制文件
  的 owner user/group ID
- saved user/group ID 都是从 effective user/group ID 复制过来的
- UNIX 提供了一系列系统调用, 可将进程自身的 effective user/group ID 在 real user/group ID 与 saved
  user/group ID 之间切换

那么可以简单推断出这几点:

- 对于非 Set-User-ID/Set-Group-ID 程序而言, 它们的 real/effective/saved user ID 与 group ID 会始终保
  持一致, 不会出现不一致的情况.
- Set-User-ID/Set-Group-ID 程序可以灵活地切换自身的 effective user/group ID, 动态调整自身的权限.

## file-system user ID 与 group ID

这是历史遗留下来的一组 ID, 目前已经很少再用到.

它通常与 effective user/group ID 一致, 因此可以简单忽略掉它.

## supplementary group IDs

前面提到的一个进程通常都只有一个 UID 与 GID, 这通常是够用了, 但无法实现一些更复杂的权限控制.

而 supplementary group IDs 就是用于实现一些更复杂的权限管控而提供的, 一个典型场景是:

- 应用程序 A B C 分别以 user/group a b c 运行
- 应用程序 A B C 都需要访问某一公共文件夹 `/data/shared`

那这种场景下最好的解决方法就是:

- 将公共文件夹的 owner group 设为 shared
- 将 shared 组添加到应用程序 A B C 的 supplementary group IDs 中

这样三个应用程序都拥有了正确的权限, 而且权限仍然控制得非常细粒度, 未牺牲任何安全性.

## 相关系统调用


| Interface(unistd.h)                                | Purpose and effect within unprivileged process                                                                                                           | Purpose and effect within privileged process                            | Portability                                                     |
| ---------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------- | --------------------------------------------------------------- |
| setuid(u)<br>setgid(g)                   | Change effective ID to the same value as current real or saved set ID                                                                                    | Change real, effective, and saved set IDs to any (single) value         | Specified in SUSv3; BSD derivatives have different semantics    |
| seteuid(e)<br>setegid(e)                 | Change effective ID to the same value as current real or saved set ID                                                                                    | Change effective ID to any value                                        | Specified in SUSv3                                              |
| setreuid(r, e)<br>setregid(r, e)         | (Independently) change real ID to same value as current real or effective ID, and effective ID to same value as current real, effective, or saved set ID | (Independently) change real and effective IDs to any values             | Specified in SUSv3, but operation varies across implementations |
| setresuid(r, e, s)<br>setresgid(r, e, s) | (Independently) change real, effective, and saved set IDs to same value as current real, effective, or saved set ID                                      | (Independently) change real, effective, and saved set IDs to any values | Not in SUSv3 and present on few other UNIX implementations      |
| setfsuid(u)<br>setfsgid(u)               | Change file-system ID to same value as current real, effective, file system, or saved set ID                                                             | Change file-system ID to any value                                      | Linux-specific                                                  |
| setgroups(n, l)                          | Can’t be called from an unprivileged process                                                                                                             | Set supplementary group IDs to any values                               | Not in SUSv3, but available on all UNIX implementations         |


