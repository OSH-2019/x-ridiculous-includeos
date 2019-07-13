# ELF 方面代码整理

## Q1. musl libc 要求何种 ELF 信息？
```c
    aux[i++].set_long(AT_PHENT, ehdr->e_phentsize);
    aux[i++].set_ptr(AT_PHDR, ((uint8_t*)ehdr) + ehdr->e_phoff);
    aux[i++].set_long(AT_PHNUM, ehdr->e_phnum);
```
AT_PHENT: The address of the program headers of the executable. 

AT_PHDR:  The size of program header entry.

AT_PHNUM: The number of program headers.

## Q2. musl libc 中此信息的用处


## Q10. 使用 Multiboot 规范后，内存中内容应该为何？ IncludeOS 原来的代码如何实现 Multiboot？
原 IncludeOS 代码中有 .multiboot 节，是哪里来的呢？
  - aarch64 没有
  - x86_pc 中有，在 `src/platform/x86_pc/start.asm` 中，摘录如下：
```
%define  MB_MAGIC   0x1BADB002
%define  MB_FLAGS   0x3  ;; ALIGN + MEMINFO

;; stack base address at EBDA border
;; NOTE: Multiboot can use 9d400 to 9ffff
%define  STACK_LOCATION     0x9D3F0

extern _MULTIBOOT_START_
extern _LOAD_START_
extern _LOAD_END_
extern _end
extern fast_kernel_start

ALIGN 4
section .multiboot
  dd  MB_MAGIC
  dd  MB_FLAGS
  dd  -(MB_MAGIC + MB_FLAGS)
  dd _MULTIBOOT_START_
  dd _LOAD_START_
  dd _LOAD_END_
  dd _end   ;; 上面五个符号都用 Linker Script 来填，参见 linker_extended.ld
  dd _start ;; 这个就是下面的 _start 符号
  ;; used for faster live updates
  dd 0xFEE1DEAD
  dd fast_kernel_start
```

（其实 aarch64 的也有这个设计，但是似乎没搞好？）

ELF Ehdr 和 Phdr 在（正常的）装载器装载后会到哪里呢？
  - 会装到第一个 Segment

## Things to do
1. 测试 QEMU 是否支持 Multiboot ELF on aarch64
  - 如果不支持，就得手写一个 Multiboot ELF loader
2. 如果成功加载，找到正确的 phdr 和 ehdr 地址，并且检查一下是否需要 _move_symbol 之类的操作


## Note: Threading in Linux
https://en.wikipedia.org/wiki/Native_POSIX_Thread_Library


