/*
测试跳转到一个已返回的函数中，会发生什么。

程序流程：根据书中描述
1. 首先 调用程序 x，并在 x 中使用 setjmp 在全局变量 env 中建立一个跳转目标
2. 从函数 x 中返回
3. 调用函数 y，在 y 中调用 longjmp 函数

此程序根据编译参数的不同，表现也有区别：
1. 使用 gcc 6_2.c 编译，能正常运行，但是打印出的最后一条信息为 `from y, a = 0`，说明 a 访问的内存被初始化为 0 了
2. 使用 gcc 6_2.c -O3 编译并提升优化等级，会导致程序进入死循环，最终产生 segmentation fault.
3. 使用 clang 编译测试也是类似的效果
*/
#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

void x()
{
    int a = 10;

    if (setjmp(env) == 0)
    {
        printf("from main, a = %d\n", a);
    } else {
        printf("from y, a = %d\n", a);
    }
}

void y()
{
    longjmp(env, 1);
}

int main()
{
    x();
    y();
    return 0;
}