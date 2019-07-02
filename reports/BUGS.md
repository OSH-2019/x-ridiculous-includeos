# BUGS 

1. QEMU 不会检查 unaligned memory access
  - 在 PowerPC 上、x86 上没事，在 ARM 上爆炸（因为 ARM 强制 Aligned Memory Access）

2. C 无符号负数升格

