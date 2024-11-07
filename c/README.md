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
export NAME=12_2
gcc $NAME.c -o $NAME.out -l tlpi  -Wall --debug
```

参数 `-l tlpi` 表示在编译时，要将我们之前放入到系统依赖目录中的 `libtlpi.a` 静态库链接进来，如果缺少这个参数，部分使用了 `tlpi_hdr.h` 的程序将无法通过编译。

使用该头文件与静态库的主要目的是与书中代码一致，写出来的代码也更简化些，降低了门槛。（实际上完全可以不依赖该静态库、不使用 `tlpi_hdr.h`）


而 `-Wall` 则表示显示所有 warning 级别的编译提示，这对判断运行期错误的位置跟原因很有帮助。

最后 `--debug` 是在编译得到的程序中添加代码相关信息，在出问题时方便调试。

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
