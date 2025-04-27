# 共享库高级特性


## 动态加载库

41 章介绍的共享库使用，都是在编译与运行层面处理的，未涉及任何程序代码的内容。

实际上 Linux 也提供了共享库读取相关的 API，使应用程序能动态地加载某个共享库，并使用符号名查找对应的函数定义，进而直接调用该函数。

动态加载一个共享库，这样的功能使程序插件机制，以及插件的热更新成为可能。

### 动态加载 API（详细声明与注释）

Linux 动态加载库的核心功能由 `dlfcn.h` 提供，以下是详细的 API 声明和说明：

#### 1. **`dlopen` - 打开共享库**
```c
/**
 * @brief 打开一个共享库文件，返回操作句柄。
 * @param filename 共享库路径。若为 NULL，则返回主程序的句柄。
 * @param flags 加载模式标志：
 *   - RTLD_LAZY: 延迟绑定（符号在首次使用时解析）。
 *   - RTLD_NOW: 立即解析所有符号。
 *   - RTLD_GLOBAL: 使库的符号全局可见。
 *   - RTLD_LOCAL: 符号仅对当前库可见（默认）。
 * @return 成功返回库的句柄，失败返回 NULL。
 */
void *dlopen(const char *filename, int flags);
```

#### 2. **`dlsym` - 查找符号地址**
```c
/**
 * @brief 通过符号名查找函数或变量的地址。
 * @param handle 库句柄（来自 dlopen 或特殊值 RTLD_DEFAULT/RTLD_NEXT）。
 * @param symbol 符号名称（如函数或变量名）。
 * @return 成功返回符号地址，失败返回 NULL。
 */
void *dlsym(void *handle, const char *symbol);
```

#### 3. **`dlclose` - 关闭共享库**
```c
/**
 * @brief 关闭共享库句柄，减少引用计数。
 * @param handle 库句柄。
 * @return 成功返回 0，失败返回非零（如库仍有未释放的符号）。
 */
int dlclose(void *handle);
```

#### 4. **`dlerror` - 获取错误信息**
```c
/**
 * @brief 获取动态加载过程中的错误信息。
 * @return 返回最近一次错误的字符串描述，若无错误则返回 NULL。
 * @note 每次调用会清空错误状态，后续调用将返回 NULL。
 */
char *dlerror(void);
```

#### 特殊句柄说明
- **`RTLD_DEFAULT`**: 默认搜索范围（全局符号表）。
- **`RTLD_NEXT`**: 查找下一个匹配的符号（用于“包装”函数）。
```c
// 示例：查找下一个 "read" 函数
void *next_read = dlsym(RTLD_NEXT, "read");
```

#### 线程安全
- 这些函数是线程安全的，但 `dlerror` 的返回值是静态缓冲区，需避免多线程竞争。


#### 示例代码
```c
#include <dlfcn.h>
#include <stdio.h>

int main() {
    void *handle = dlopen("./libfoo.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    void (*func)() = dlsym(handle, "foo");
    if (!func) {
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    func(); // 调用动态加载的函数
    dlclose(handle);
    return 0;
}
```


## 控制共享库中定义的符号可见性

共享库中的符号（函数、变量）默认是全局可见的，但可以通过以下方式控制其可见性，避免命名冲突和隐藏内部实现细节。

### 1. **使用 `static` 关键字**
将符号声明为 `static`，使其仅在当前编译单元（文件）内可见：
```c
static void internal_function() {
    // 仅在此文件内可见
}
```

### 2. **GCC 的 `-fvisibility` 选项**
通过编译选项 `-fvisibility=hidden` 默认隐藏所有符号，再显式导出需要的符号：
```bash
gcc -fvisibility=hidden -shared -o libfoo.so foo.c
```

#### 显式导出符号
在代码中使用 `__attribute__((visibility("default")))` 标记需要导出的符号：
```c
void __attribute__((visibility("default"))) public_function() {
    // 导出此函数
}
```

### 3. **版本脚本**
使用链接器脚本（`.map` 文件）精确控制符号的可见性和版本：
```map
{
    global: public_function;
    local: *;
};
```
编译时指定版本脚本：
```bash
gcc -shared -o libfoo.so foo.c -Wl,--version-script=libfoo.map
```

### 4. **`__attribute__((weak))`**
将符号标记为弱引用，允许运行时覆盖：
```c
void __attribute__((weak)) weak_function() {
    // 可被其他实现覆盖
}
```

### 注意事项
- **性能**: 隐藏符号可以减少动态链接时的查找时间。  
- **兼容性**: 确保导出的符号与 API 设计一致，避免破坏二进制兼容性。

## 使用链接器脚本创建版本化的符号

通过链接器脚本（Linker Script），可以为共享库中的符号添加版本信息，实现符号的多版本共存和向后兼容。

### 1. **版本脚本示例**
创建一个版本脚本文件（如 `libfoo.map`）：
```map
LIBFOO_1.0 {
    global:
        foo_v1;
        bar;
    local:
        *; // 隐藏其他符号
};

LIBFOO_2.0 {
    global:
        foo_v2;
} LIBFOO_1.0; // 继承旧版本
```

### 2. **编译共享库**
在编译时指定版本脚本：
```bash
gcc -shared -o libfoo.so foo.c -Wl,--version-script=libfoo.map
```

### 3. **运行时行为**
- 程序默认使用最新版本的符号（如 `foo_v2`）。  
- 旧程序仍能链接到 `LIBFOO_1.0` 的符号（如 `foo_v1`），确保兼容性。

### 4. **检查符号版本**
使用 `nm` 或 `objdump` 查看符号版本：
```bash
nm -D libfoo.so | grep foo
```

### 注意事项
- **兼容性**: 版本化符号需严格遵循语义化版本（SemVer）原则。  
- **调试**: 版本冲突时，使用 `LD_DEBUG=bindings` 调试动态链接过程。


## 动态加载库的初始化与终止函数

共享库可以定义初始化函数和终止函数，分别在库被加载和卸载时自动执行，用于资源管理或状态设置。

### 1. **构造函数（Initialization）**
使用 `__attribute__((constructor))` 定义初始化函数：
```c
__attribute__((constructor)) 
void init() {
    printf("Library loaded\n");
}
```

### 2. **析构函数（Termination）**
使用 `__attribute__((destructor))` 定义终止函数：
```c
__attribute__((destructor)) 
void cleanup() {
    printf("Library unloaded\n");
}
```

### 3. **优先级**
可以为构造函数/析构函数指定优先级（数字越小，执行越早）：
```c
__attribute__((constructor(101))) void init_early() {}
__attribute__((destructor(101))) void cleanup_late() {}
```

### 4. **注意事项**
- **执行顺序**: 构造函数按优先级升序执行，析构函数按降序执行。  
- **线程安全**: 初始化函数需考虑多线程环境下的竞态条件。  
- **避免依赖**: 构造函数中避免依赖其他未初始化的库。

## 使用 LD_DEBUG 来监控动态链接器的操作

`LD_DEBUG` 是一个环境变量，用于动态链接器的调试，可以输出详细的链接过程信息，帮助诊断符号解析、库加载等问题。

### 1. **常用调试选项**
设置 `LD_DEBUG` 为以下值之一：
- **`files`**: 显示库的加载过程。  
- **`bindings`**: 显示符号绑定信息。  
- **`symbols`**: 显示符号查找过程。  
- **`all`**: 启用所有调试输出（可能非常冗长）。

#### 示例
```bash
LD_DEBUG=bindings ./my_program
```

### 2. **输出重定向**
将调试信息输出到文件：
```bash
LD_DEBUG=files LD_DEBUG_OUTPUT=debug.log ./my_program
```

### 3. **常见问题诊断**
- **符号未找到**: 检查 `LD_DEBUG=bindings` 的输出，确认符号是否在预期库中。  
- **库路径问题**: 使用 `LD_DEBUG=files` 查看库搜索路径。  
- **版本冲突**: 结合 `LD_DEBUG=bindings` 和 `nm` 工具检查符号版本。

### 4. **其他工具**
- **`ldd`**: 查看程序的库依赖关系。  
- **`objdump`**: 分析库的符号表和版本信息。  
- **`strace`**: 跟踪系统调用，辅助诊断加载失败问题。

