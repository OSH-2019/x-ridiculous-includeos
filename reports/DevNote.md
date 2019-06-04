# 开发日志

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
  - Note that cell codes are still having problems, maybe we need to feed qemu with overlayed dtbs, instead of only root ones
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
