
# The Linux Programming Interface 习题解答


## 环境搭建

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

## 编译程序

以 `4_1.c` 为例介绍编译方法：

```shell
gcc 7_1.c -o 7_1.out -l tlpi
```

参数 `-l tlpi` 表示在编译时，要将我们之前放入到系统依赖目录中的 `libtlpi.a` 静态库链接进来，如果缺少这个参数，部分使用了 `tlpi_hdr.h` 的程序将无法通过编译。

使用该头文件与静态库的主要目的是与书中代码一致，写出来的代码也更简化些，降低了门槛。（实际上完全可以不依赖该静态库、不使用 `tlpi_hdr.h`）


## 参考

- https://github.com/sunhuiquan/tlpi-learn
