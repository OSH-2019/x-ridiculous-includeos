# 开发日志

2019-07-05:
- 增加了对屏幕的支持，需要时可以在屏幕上打印（日志之类的）了。
- The whole musl things work fine, given `-s build_type=Debug` set while doing `conan create . musl/1.1.18@includeos/stable -pr gcc-8.2.0-linux-aarch64 -s build_type=Debug`
  - But why????

2019-07-02:
- 还是没有成功启动😂
- 明天需要整合 USB
- SD 和 file system 需要整合  

2019-06-27:
- 写了一点点 ELF Parser 的东西
- ~~Civ5 和 Fate of the Dragon 真好玩~~

2019-06-26:
- 开始搞 sdcard 的移植（yjh & tyl）
- 正在试图搞懂 tls 的相关逻辑；考虑 ELF 直接加载的可行方案
  - U-Boot 不行，kernel image 的格式也不是 ELF
  - 应该要手写... Multiboot binary bootloader
  - 《程序员的自我修养-链接、装载与库》比较好用

2019-06-07:
- Fix the bugs in GPIO module and need to add the GPIO driver into includeOS.
- (LZT: Examine the ELF header and auxiliary vectors. 咕咕ed others and apologized for his behaviour.)

2019-06-06:
- Starting the GPIO module but still have bugs.

2019-06-05:
- Skip the `__libc_start_main` executing `kernel_main` instead.
- `global_ctors_ok == 42` check failed and temply skip.
  ```
  //LL_ASSERT(global_ctors_ok == 42);
  SYN EXCEPTION 02000000
  ```
- `kernel_sanity_checks` check failed and temply skip.
  ```
  Global constuctors not working (or modified during run time).
  SYN EXCEPTION 97800010
  ```
- Replace `cout` in `main.cpp` with `printf` and finally print `Hello world` on the screen.
  ```
  ZT debugging
  Magic 3f000000 addrin 8
  CurrentEL 00000002
  size_cells : 00000001
  addr_cells : 00000001
  mem_offset : 00000090
  RAM BASE : 0000000000000000
  RAM SIZE : 000000003C000000
  [aarch64 PC] constructor 
  [ Machine ] Initializing heap
  [ Machine ] Main memory detected as 1005104960 b
  [ Machine ] Reserving 1048576 b for machine use 
  * Initializing aux-vector @ 0x7fd28
  * Stack protector value: 0
  * Starting libc initialization
  <kernel_main> libc initialization complete 
  <kernel_main> OS start 
  <kernel_main> sanity checks 
  <kernel_main> post start 
  ================================================================================
                                                                                IncludeOS 0.14.2-1211 (aarch64 / 64-bit)
                                         +--> Running [ Hello world - OS included ]
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    +--> WARNING: No good random source found: RDRAND/RDSEED instructions not available.
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Hello world
                          [ main ] returned with status 0
                                                         <kernel_main> os_event_loop
  ```
  
2019-06-04:
- Figured out how dtb works
- After qemu singlestepping & [u-boot code](https://github.com/u-boot/u-boot/blob/master/board/raspberrypi/rpi/lowlevel_init.S) I found dtb addr stored in x0 at boot time
  - `qemu-system-aarch64 -M raspi3 -S -gdb tcp::2333 -kernel test.img.14 -d in_asm -dtb ../../../ArchLinuxARM/boot/dtbs/broadcom/bcm2837-rpi-3-b-plus.dtb`
  - Now debugging messages look like below:
```
ZT debugging
Magic 3f000000 addrin 8
CurrentEL 00000002
size_cells : 
addr_cells : 
mem_offset : 
RAM BASE : 
RAM SIZE : 
[aarch64 PC] constructor 
[ Machine ] Initializing heap
[ Machine ] Main memory detected as 1005092672 b
[ Machine ] Reserving 1048576 b for machine use 
* Elf start: 0x7ff18
/home/libreliu/OS/IncludeOS-dev-new/modified_src_editable/src/../api/util/elf_binary.hpp:56:Elf_binary Expects failed: is_ELF() 
SYN EXCEPTION 97800010
```

```
ZT debugging
Magic 3f000000 addrin 8
CurrentEL 00000002
size_cells : 
addr_cells : 
mem_offset : 
RAM BASE : 
RAM SIZE : 
[aarch64 PC] constructor 
[ Machine ] Initializing heap
[ Machine ] Main memory detected as 1005092672 b
[ Machine ] Reserving 1048576 b for machine use 
* Initializing aux-vector @ 0x7fd38
* Stack protector value: 0
* Starting libc initialization
```
  - Note that cell codes are still having problems, maybe we need to feed qemu with overlayed dtbs, instead of only root ones
  - Seem like the libc could not work.
-----
2019-06-03:
- 学习了 AArch64 Exception，分析了 exception.asm
- 定位并修复了访问栈时产生异常的问题
  - 修改 linker.ld 的起始地址
- 把 __serial_print 函数替换为可工作版本
- 了解 QEMU 的工作机制
-----
2019-06-02:
- build / package 文件夹的区别？一个有 aarch64/linker.ld，一个直接是 linker.ld
  - 应该是因为 -bf 是构建文件夹， -pf (package folder) 是 cmake install 到的文件夹

-----
2019-06-01:
- 把 raspi3-tutorial 的 uart 例程整体平移到 `src/arch/aarch64` 中, 编译成功了
  - 在 `arch_start.asm` 中增加跳转
  - 在 `linker.ld` 中增加 .bss 段大小计算
  - 改掉初始 sp 地址(随便写了一个,但是原来的不行,被占用了)
