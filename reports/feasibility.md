# includeOS on ARM 可行性报告

## 项目介绍
IncludeOS 是一个 C++ 的 Unikernel 实现，并可以在 bare-metal 上运行。IncludeOS 提供了丰富的用于网络编程的库，但是目前还不支持在 ARM 上运行。裸机运行的 IncludeOS 相较于 Linux 发行版拥有更快的启动速度，并且减少了进程切换等的无谓开销。现有的树莓派的 Unikernel 主要是对一些开关 GPIO 等相关的实现，但是对网络的支持很弱。在 IoT 领域中，有许多应用场景对延迟的要求十分苛刻，而本项目意在将 IncludeOS 移植到 ARM 上，这样对延迟敏感的 IoT 应用场景会有很大帮助。

## 理论依据


## 技术依据

在「技术依据」章节，我们将对 includeOS 的开发环境配置，启动过程和相关技术进行简要的介绍。

### includeOS 环境配置
Ref: [GetStarted @ IncludeOS](http://www.includeos.org/get-started.html)

includeOS 会将操作系统相关的代码，以及链接和测试工具安装到系统中。默认的安装目录是`/usr/local/includeos`。

对于一般开发者来说，可能更希望使用一个自定义的目录，这可以通过设置环境变量解决。此处是我们写的一个比较方便的脚本：

```shell
#!/bin/bash

# IncludeOS tools & static labs will be located here
export INCLUDEOS_PREFIX=`pwd`/IncludeOS_Install/
export PATH=$PATH:$INCLUDEOS_PREFIX/bin
export CC="clang"
export CXX="clang++"

echo "New Env Var: $PATH"
echo "In IncludeOS_Install Env Now"
echo 'export PS1="[\u@ios-env(\h) \W]\$ "' > bash_start_arm.rc
bash --init-file bash_start_arm.rc -i
echo "Exited from IncludeOS_Install Env"
```
在配置好环境变量之后，就可以进入安装：
```
git clone "https://github.com/includeos/IncludeOS"
cd IncludeOS
./install.sh
```
安装以 Fedora, Arch, Ubuntu 和 Debian 为宜。安装过程中注意安装依赖。`vmrunner`等脚本都是 Python 2 的，安装时请提前装好 Python 2 对应的包，否则按 ImportError 的指示来安装也是可以的。

*Notice:* 在创建网桥时默认采用的是 `ifconfig` 命令（参`./etc/scripts/create_bridge.sh`）。现在的发行版较少默认安装；Arch 用户可以使用`sudo pacman -Sy net-tools`进行安装。

安装好后可以运行`./test.sh`（位于 repo 根目录下）来测试编译 Example Service 能否通过，并且能否在 QEMU 中运行。

### includeOS Demo
includeOS 的开发体验非常便捷。只需要复制`seed/service`文件夹到任意位置，并且对`Service::start`函数进行更改即可。

`CMakeLists.txt` 中的 `SERVICE_NAME` 和 `BINARY` 字段也可更改，会生成不同的 binary 文件名。

示例的 service 如下所示：
```c
#include <service>
#include <cstdio>
#include <isotime>
#include <kernel/cpuid.hpp>

void Service::start(const std::string& args)
{
#ifdef __GNUG__
  printf("Built by g++ " __VERSION__ "\n");
#endif
  printf("Hello world! Time is now %s\n", isotime::now().c_str());
  printf("Args = %s\n", args.c_str());
  printf("Try giving the service less memory, eg. 5MB in vm.json\n");
  printf("CPU has RDRAND: %d\n", CPUID::has_feature(CPUID::Feature::RDRAND));
  printf("CPU has RDSEED: %d\n", CPUID::has_feature(CPUID::Feature::RDSEED));
}
```

完成后运行以下命令：
```bash
mkdir build && cd build
cmake ..
make
boot my_service
```
即可打开 QEMU，并且以`my_service`这个 binary 启动。

### includeOS 构建过程
Ref: [The Build Process](https://includeos.readthedocs.io/en/latest/The-build-process.html)

Installing IncludeOS means building all the OS components, such as IRQ manager, PCI manager, the OS class etc., combining them into a static library os.a using GNU ar, and putting it in an architecture specific directory under $INCLUDEOS_PREFIX along with all the public os-headers (the “IncludeOS API”). This is what you’ll be including parts of, into the service. Device drivers are built as their own libraries, and must be explicitly added in the CMakeLists.txt of your service. This makes it possible to only include the drivers you want, while still not having to explicitly mention a particular driver in your code.

安装 IncludeOS 意味着构建所有 OS 组件，比如 IRQ 管理器，PCI 管理器，OS 类——把他们用 GNU `ar` 整合到一个静态库 `os.a` 中，然后

When the service gets built it will turn into object files, which eventually gets statically linked with the os-library, drivers, plugins etc. It will also get linked with the pre-built standard libraries (libc.a, libc++.a etc.) which we provide as a downloadable bundle, pre-built using this script. Only the objects actually needed by the service will be linked, turning it all into one minimal elf-binary, your_service, with OS included.

This binary contains a multiboot header, which has all the information the bootloader needs to boot it. This gives you a few options for booting, all available through the simple boot tool that comes with IncludeOS:

Qemu kernel option: For 32-bit ELF binaries qemu can load it directly without a bootloader, provided a correct multiboot header. This is what boot <service path> will do by default. The boot tool will generate something like $ qemu_system_x86_64 -kernel your_service ..., which will boot your service directly. Adding -nographic will make the serial port output appear in your terminal. For 64-bit ELF binaries Qemu has a paranoid check that prevents this, so we’re using a 32-bit IncludeOS as chainloader for that. If boot <service path> detects a 64-bit ELF it will use the 32-bit chainloader as -kernel, and add the 64 bit binary as a “kernel module”, e.g. -initrd <my_64_bit_kernel>. The chainloader will copy the 64-bit binary to the appropriate location in memory, modify the multiboot info provided by the bootloader to the kernel, and jump to the new kernel, which boots as if loaded directly by e.g. GRUB.
Legacy: Attach our own minimal bootloader, using the utility vmbuild. It combines our minimal bootloader and your_service binary into a disk image called your_service.img. At this point the bootloader gets the size and location of the service hardcoded into it. The major drawback of using this bootloader is that it doesn’t fetch information about system memory from the BIOS so you can’t know exactly how much memory you have, above 65Mb. (Which CMOS can provide)
Grub: Embed the binary into a GRUB filesystem, and have the Grub chainloader boot it for you. This is what we’re doing when booting on Google Compute Engine. You can do this on Linux using boot -g <service path>, which will produce a bootable your_service.grub.img. Note that GRUB is larger than IncludeOS itself, so expect a few megabytes added to the image size.
To run with vmware or virtualbox, the image has to be converted into a supported format, such as vdi or vmdk. This is easily done in one command with the qemu-img-tool, that comes with Qemu. We have a script for that too. Detailed information about booting in vmware, which is as easy as boot, is provided here.

Inspect the main CMakeLists.txt and then follow the trail of cmake scripts in the added subfolders for information about how the OS build happens. For more information about building individual services, check out the CMakeLists.txt of one of the example services, plus the linker script, linker.ld for the layout of the final binary. Note that most of the CMake magic for link- and include paths, adding drivers, plugins etc. is tucked away in the post.service.cmake.

### includeOS 启动过程（x86）

1. When booting from a "hard drive", BIOS loads the first stage bootloader, either grub or [bootloader.asm](https://github.com/hioa-cs/IncludeOS/blob/master/src/platform/x86_pc/boot/bootloader.asm), starting at `_start`.
2. The bootloader - or Qemu with `-kernel` - sets up segments, switches to 32-bit protected mode, loads the service (an elf-binary `your_service` consisting of the OS classes, libraries and your service) from disk. For a multiboot compliant boot system (grub or qemu -kernel) the machine is now in the state [specified by multiboot](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Machine-state).
3. The bootloader hands over control to the OS, by jumping to the `_start` symbol inside [start.asm](https://github.com/hioa-cs/IncludeOS/blob/master/src/platform/x86_pc/start.asm#L61). From there it will call architecture specific initialization and eventually [kernel_start.cpp](https://github.com/hioa-cs/IncludeOS/blob/master/src/platform/x86_pc/kernel_start.cpp). Note that this can be overridden to make custom kernels, such as the minimal [x86_nano](https://github.com/hioa-cs/IncludeOS/blob/master/src/platform/x86_nano) platform used for the chainloader.
4. The OS initializes `.bss`, calls global constructors, and then calls `OS::start` in the [OS class](https://github.com/hioa-cs/IncludeOS/blob/master/api/kernel/os.hpp).
5. The OS class sets up interrupts, initializes devices, plugins, drivers etc.
6. Finally the OS class (still `OS::start`) calls `Service::start()` (as for instance [here](https://github.com/hioa-cs/IncludeOS/blob/master/examples/demo_service/service.cpp)) or `main()` if you prefer that (such as [here](https://github.com/hioa-cs/IncludeOS/blob/master/examples/syslog/service.cpp)), either of which must be provided by your service.
7. Once your service is done initializing, e.g. having indirectly subscribed to certain events like incoming network packets by setting up a HTTP server, the OS resumes the [OS::event_loop()](https://github.com/hioa-cs/IncludeOS/blob/master/src/kernel/os.cpp) which again drives your service.

### includeOS Digest
说明：(external) 表示此符号/函数不由当前文件提供。

首先，由 Multiboot Bootloader 装载后的二进制映像文件从`_start`开始执行。

#### `_start` @ `src/platform/x86_pc/start.asm`
`_start`装入一个简单的 GDT，设置段寄存器 cx，ss，ds，es，fs，gs，设置 esp 和 ebp。

调用 `enable_sse`，`enable_fpu_native`，`enable_xsave`和`enable_avx`，分别启用 SSE，现代 x87 FPU 异常处理，如果 CPU 支持会再启用`xsave`和`avx`。

完成后首先保存 eax 和 ebx 到`__multiboot_magic`(global)和 `__multiboot_addr`(global)中，然后调用`__arch_start`(external)，如果从`__arch_start`(external)返回则会调用`__start_panic`。

`__start_panic` 将会调用`__serial_print1`(external)，并传参「Panic: OS returned to x86 start.asm. Halting\n」。

------

Providers:
- `__arch_start`: `src/arch/i686/arch_start.asm`, `src/arch/x86_64/arch_start_broken.asm`, `src/arch/x86_64/arch_start_broken1.asm`, `src/platform/x86_pc/start.asm`, `src/arch/x86_64/arch_start.asm`
- `__serial_print1`: `src\platform\x86_pc\serial1.cpp`

#### `__arch_start` @ `src/arch/x86_64/arch_start.asm`
*Note*: 
- `global __arch_start:function` 中 `:function` 的作用：[NASM Docs](https://nasm.us/xdoc/2.14.03rc2/html/nasmdoc6.html)
- `loop label`：用 ECX 控制循环次数，相当于`dec ecx; jne label`。
- 除了`x86_64`，其它的`arch/`里面也有`arch_start.asm`。

```
  GLOBAL, like EXTERN, allows object formats to define private extensions by means of a colon. The elf object format, for example, lets you specify whether global data items are functions or data:
  global  hashlookup:function, hashtable:data
```

总之是 ELF 可以区分符号类型，所以 NASM 加上了这个功能。

-------

`__arch_start`一开始是 32 位代码段，用`[BITS 32]`标识。
1. 关闭老的分页机制，设置页表，开启 PAE（物理地址扩展）
2. 启用 `long mode`，启动分页，加载 64-bit GDT
3. 跳转到 64 位代码段 `long_mode`

`long_mode`：
1. cli （Clear Interrupt Flag，关中断）
2. 重新设置段寄存器，装入 GDT64.Data 选择子
3. 设置新栈 rsp，rbp
4. 设置临时 SMP 表
5. "geronimo!"：edi <= DWORD[__multiboot_magic](external); esi <= DWORD[__multiboot_addr](external)
6. 调用 `kernel_start`(external)

---------

Providers:
- `kernel_start`: `src/platform/x86_solo5/kernel_start.cpp`, `src/platform/x86_nano/kernel_start.cpp`, `src/platform/x86_pc/kernel_start.cpp`
- `__multiboot_magic`: `src/platform/x86_pc/start.asm`, `src/platform/x86_solo5/start.asm`

#### `kernel_start` @ `src\platform\x86_pc\kernel_start.cpp`

*Note*:
- `__attribute__((no_sanitize("all")))` 用来「suppress warning」
- `PRATTLE` 宏在 `KERN_DEBUG` 宏被定义的情况下直接调用`kprintf`(@ `src\platform\x86_pc\serial1.cpp`)；没有定义 `KERN_DEBUG` 则什么也不干。
- `.bss`(Block Started by Symbol)：在采用段式内存管理的架构中，`.bss` 段通常指存放程序中未初始化的全局变量的一块内存区域。
- 「自制 `assert`」：`#define Expects(X) if (!(X)) { kprint("Expect failed: " #X "\n");  asm("cli;hlt"); }`

------------

`kernel_start` 是 `extern "C"` 的一个函数。有两个参数`(uint32_t magic, uint32_t addr)`（就是之前传过来的 Multiboot Magic）

1. ` __init_serial1()` @ `src\platform\x86_pc\serial1.cpp`
2. 保存 `magic` 和 `addr` 到 `__grub_magic` 和 `__grub_addr`
3. `__init_sanity_checks()` @ `src\platform\x86_pc\sanity_checks.cpp` ，用来验证内核完整性（CRC 校验）
4. 检测`free_mem_begin`和`memory_end`，获取可用空余内存地址——通过 Multiboot 引导器 / Softreset 的信息
   (emmm `OS::memory_end()` 是什么？）
5. 为了保护 ELF 中的符号信息，调用`_move_symbols`移动符号，再将返回值加到`free_mem_begin`上（保护起来！）
6. 调用`_init_bss()` 初始化 .bss 段
7. 调用`OS::init_heap(free_mem_begin, memory_end)`(@ `src/kernel/heap.cpp`) 初始化堆
8. 调用`_init_syscalls()` 初始化系统调用
9. 调用`x86::idt_initialize_for_cpu(0);` 初始化 CPU Exceptions。
10. 调用`_init_elf_parser()` 初始化 ELF Parser
11. 

### includeOS 源码概览


## 技术路线

## 参考文献
1. [Introduction to X64 Assembly](https://software.intel.com/en-us/articles/introduction-to-x64-assembly)