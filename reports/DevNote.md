# 开发日志

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
