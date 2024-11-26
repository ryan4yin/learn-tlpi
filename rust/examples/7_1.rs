// 7_1 需要调用 libc 的 unsafe 函数分配堆内存, 并在每次 malloc 之后打印出 program break 的当前值.
// 指定一个较小的内存分配尺寸, 使我们能观察到 malloc 不会每次都只申请我们指定的内存大小, 而是会有一定的内存对齐与预先分配.

use nix::libc::{malloc, sbrk, size_t};
use std::ptr;

fn get_brk() -> *mut std::ffi::c_void {
    // 调用 libc 的 sbrk 函数获取 program break 的当前值
    let brk = unsafe { sbrk(0) };
    if brk.is_null() {
        panic!("sbrk failed");
    }
    brk
}

fn _malloc(size: size_t) -> *mut std::ffi::c_void {
    // 调用 libc 的 malloc 函数分配内存
    let p = unsafe { malloc(size) };
    if p.is_null() {
        panic!("malloc failed");
    }
    p
}

fn main() {
    let sizes = [
        1024,     // 1KB
        10240,    // 10KB
        102400,   // 100KB
        1024000,  // 1MB
        10240000, // 10MB
    ]; // 指定一系列内存分配尺寸
    const NUM_ALLOC: usize = 10; // 指定分配次数

    for size in sizes.iter() {
        for i in 0..NUM_ALLOC {
            let _ptr = _malloc(*size as size_t);
            let brk = get_brk();
            println!("[{}]malloc({}) = {:p}, sbrk(0) = {:p}", i, size, _ptr, brk);
            unsafe {
                ptr::write_bytes(_ptr, 0, *size as usize); // 将分配的内存清零
            }
        }
        println!("================================")
    }
}
