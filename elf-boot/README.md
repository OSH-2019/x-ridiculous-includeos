# ELF-Boot: A minimal, Multiboot compatible ELF binary loader

## Startup process
start.S => main.c

## `Synchronous: Data abort, same EL, Address size fault at level 0:` at `move()` 问题
发现 `real_load` 在 `header_addr > load_addr` 时会成为 `0x00000001 xxxxxxxx`。

这是因为，考虑如下代码：
```c
unsigned int a = 1 - 2;
unsigned long b = 2;
printf("%lx\n", a + b);
```
因为 unsigned int + unsigned long 相加时，会把 unsigned int 做无符号扩展，然后和 unsigned long 相加。

所以结果补码相加的进位被记录了下来。

## `Synchronous: Unknown Error` 问题
在切换 EL 时可能目标 EL 没有开启 SIMD/FP 指令，导致如 `str q0, [sp, #96]` 的指令出现异常。

（Q0 为一个 SIMD 寄存器）

## `Relocation truncated to fit` 问题
```c
extern long _binary_hello_elf_bin_size;
putint(_binary_hello_elf_bin_size);  // Won't work
putint(&_binary_hello_elf_bin_size); // Will work
```
Error Message: (Something like this)
```
main.c:(.text.startup+0x24): relocation truncated to fit: R_AARCH64_LDST64_ABS_LO12_NC against symbol `_binary_bcm2837_rpi_3_b_plus_dtb_size' defined in *ABS* section in kernel8.elf
aarch64-linux-gnu-ld: main.c:(.text.startup+0x24): warning: one possible cause of this error is that the symbol is being referenced in the indicated code as if it had a larger alignment than was declared where it was defined
```
In the manual:
```
286 | R_AARCH64_LDST64_ABS_LO12_NC | S + A | Set the LD/ST immediate value to bits [11:3] of X. No overflow check.
```
In assembly (the wrong version):
```
[libreliu@thinkpad-ssd elf-boot]$ aarch64-linux-gnu-objdump -d main.o

main.o:     file format elf64-littleaarch64


Disassembly of section .text.startup:

0000000000000000 <main>:
   0:   a9bf7bfd        stp     x29, x30, [sp, #-16]!
   4:   90000000        adrp    x0, 0 <main>
   8:   91000000        add     x0, x0, #0x0
   c:   910003fd        mov     x29, sp
  10:   94000000        bl      0 <printf_>
  14:   90000000        adrp    x0, 0 <_binary_bcm2837_rpi_3_b_plus_dtb_size>
  18:   90000002        adrp    x2, 0 <_binary_bcm2837_rpi_3_b_plus_dtb_end>
  1c:   90000001        adrp    x1, 0 <_binary_bcm2837_rpi_3_b_plus_dtb_start>
  20:   f9400003        ldr     x3, [x0]
  24:   90000000        adrp    x0, 0 <main>
  28:   f9400042        ldr     x2, [x2]
  2c:   91000000        add     x0, x0, #0x0
  30:   f9400021        ldr     x1, [x1]
  34:   94000000        bl      0 <printf_>
  38:   90000000        adrp    x0, 0 <_binary_hello_elf_bin_size>
  3c:   90000002        adrp    x2, 0 <_binary_hello_elf_bin_end>
  40:   90000001        adrp    x1, 0 <_binary_hello_elf_bin_start>
  44:   a8c17bfd        ldp     x29, x30, [sp], #16
  48:   f9400003        ldr     x3, [x0]
  4c:   90000000        adrp    x0, 0 <main>
  50:   f9400042        ldr     x2, [x2]
  54:   91000000        add     x0, x0, #0x0
  58:   f9400021        ldr     x1, [x1]
  5c:   14000000        b       0 <printf_>
```
Relocations for the wrong version:
```
[libreliu@thinkpad-ssd elf-boot]$ aarch64-linux-gnu-objdump -r main.o

main.o:     file format elf64-littleaarch64

RELOCATION RECORDS FOR [.text.startup]:
OFFSET           TYPE              VALUE 
0000000000000004 R_AARCH64_ADR_PREL_PG_HI21  .rodata.str1.8
0000000000000008 R_AARCH64_ADD_ABS_LO12_NC  .rodata.str1.8
0000000000000010 R_AARCH64_CALL26  printf_
0000000000000014 R_AARCH64_ADR_PREL_PG_HI21  _binary_bcm2837_rpi_3_b_plus_dtb_size
0000000000000018 R_AARCH64_ADR_PREL_PG_HI21  _binary_bcm2837_rpi_3_b_plus_dtb_end
000000000000001c R_AARCH64_ADR_PREL_PG_HI21  _binary_bcm2837_rpi_3_b_plus_dtb_start
0000000000000020 R_AARCH64_LDST64_ABS_LO12_NC  _binary_bcm2837_rpi_3_b_plus_dtb_size
0000000000000024 R_AARCH64_ADR_PREL_PG_HI21  .rodata.str1.8+0x0000000000000020
0000000000000028 R_AARCH64_LDST64_ABS_LO12_NC  _binary_bcm2837_rpi_3_b_plus_dtb_end
000000000000002c R_AARCH64_ADD_ABS_LO12_NC  .rodata.str1.8+0x0000000000000020
0000000000000030 R_AARCH64_LDST64_ABS_LO12_NC  _binary_bcm2837_rpi_3_b_plus_dtb_start
0000000000000034 R_AARCH64_CALL26  printf_
0000000000000038 R_AARCH64_ADR_PREL_PG_HI21  _binary_hello_elf_bin_size
000000000000003c R_AARCH64_ADR_PREL_PG_HI21  _binary_hello_elf_bin_end
0000000000000040 R_AARCH64_ADR_PREL_PG_HI21  _binary_hello_elf_bin_start
0000000000000048 R_AARCH64_LDST64_ABS_LO12_NC  _binary_hello_elf_bin_size
000000000000004c R_AARCH64_ADR_PREL_PG_HI21  .rodata.str1.8+0x0000000000000040
0000000000000050 R_AARCH64_LDST64_ABS_LO12_NC  _binary_hello_elf_bin_end
0000000000000054 R_AARCH64_ADD_ABS_LO12_NC  .rodata.str1.8+0x0000000000000040
0000000000000058 R_AARCH64_LDST64_ABS_LO12_NC  _binary_hello_elf_bin_start
000000000000005c R_AARCH64_JUMP26  printf_


RELOCATION RECORDS FOR [.eh_frame]:
OFFSET           TYPE              VALUE 
000000000000001c R_AARCH64_PREL32  .text.startup
```
`*.dto` symtab: (Some outputs are truncated, buggy!)
```
[libreliu@thinkpad-ssd elf-boot]$ aarch64-linux-gnu-readelf -s *.dto

Symbol table '.symtab' contains 5 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 
     2: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT    1 _binary_bcm2837_rpi_3_b_p
     3: 0000000000004b2d     0 NOTYPE  GLOBAL DEFAULT    1 _binary_bcm2837_rpi_3_b_p
     4: 0000000000004b2d     0 NOTYPE  GLOBAL DEFAULT  ABS _binary_bcm2837_rpi_3_b_p
```
`main.o` symtab: (Example, not fully fitted in the scenario)
```
[libreliu@thinkpad-ssd elf-boot]$ aarch64-linux-gnu-readelf -s main.o

Symbol table '.symtab' contains 21 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS main.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    2 
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    4 
     6: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT    4 $x
     7: 0000000000000000     0 SECTION LOCAL  DEFAULT    6 
     8: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT    6 $d
     9: 0000000000000000     0 SECTION LOCAL  DEFAULT    8 
    10: 0000000000000014     0 NOTYPE  LOCAL  DEFAULT    9 $d
    11: 0000000000000000     0 SECTION LOCAL  DEFAULT    9 
    12: 0000000000000000     0 SECTION LOCAL  DEFAULT    7 
    13: 0000000000000000    96 FUNC    GLOBAL DEFAULT    4 main
    14: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND printf_
    15: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_bcm2837_rpi_3_b_p
    16: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_bcm2837_rpi_3_b_p
    17: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_bcm2837_rpi_3_b_p
    18: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_hello_elf_bin_siz
    19: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_hello_elf_bin_end
    20: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _binary_hello_elf_bin_sta
```
注：ADRP Xn, val 相当于 Xn = Val << 12 + (PC & 0xfffff000)
   「其实是取当前 PC 对应的页表的基地址，加上 Val << 12 （即 Val 个页）」的地址
   下面一般要用 Add 再加上一些，得到真正的地址。

下面尝试解答一下这个问题：
emmm感觉还是不太对...