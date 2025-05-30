# The Linux Programming Interface 习题解答


## 一、环境搭建

### 1. NixOS

只需要在此文件夹中运行如下命令即可进入开发环境：

```shell
nix develop
```

### 2. Non-NixOS Linux

安装 tlpi 依赖库：

```shell
# 下载编译 tlpi 依赖库
wget https://man7.org/tlpi/code/download/tlpi-201025-dist.tar.gz
tar -zxvf tlpi-201025-dist.tar.gz
cd tlpi-dist/
make

# 将书中依赖库与头文件 copy 到系统的依赖目录，方便引用
sudo cp libtlpi.a /usr/local/lib
cd lib/
sudo cp tlpi_hdr.h /usr/local/include/
sudo cp get_num.h /usr/local/include/
sudo cp error_functions.h /usr/local/include/
sudo cp ename.c.inc /usr/local/include/
```

安装内核相关头文件与依赖库：

```shell
# redhat/centos/fedora
yum install gcc kernel-devel glibc-devel libcap-devel libacl-devel
```

## 二、编译程序

以 `4_1.c` 为例介绍编译方法：

```shell
just nb 4_1
./4_1.out
```

## 段错误问题的排查方法

在 memset 等函数使用不当时，就容易造成段错误，而且很难通过常规的测试手段找到问题所在。

此时建议使用 valgrind 来进行问题检测：

```shell
# 安装 valgrind
yum install valgrind

# 执行测试
valgrind --leak-check=yes ./12_2.out
```

## 参考

- https://github.com/sunhuiquan/tlpi-learn
