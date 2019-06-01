# 开发日志
-----
2019-06-01:
- 把 raspi3-tutorial 的 uart 例程整体平移到 `src/arch/aarch64` 中, 编译成功了
  - 在 `arch_start.asm` 中增加跳转
  - 在 `linker.ld` 中增加 .bss 段大小计算
  - 改掉初始 sp 地址(随便写了一个,但是原来的不行,被占用了)