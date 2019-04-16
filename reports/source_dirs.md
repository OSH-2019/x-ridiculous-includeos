# IncludeOS 源码结构分析

```
.
|-- CMakeLists.txt       主 CMake 文件
|-- CONTRIBUTE.md        贡献指南（没啥用）
|-- Dockerfile           在 Ubuntu Xenial 这个 Base 下构建 IncludeOS
                         入口点 Script 在 IncludeOS/etc/docker_entrypoint.sh 
|-- LICENSE              Apache 开源协议 v2.0
|-- NOTICE               
|-- NaCl                 NaCl 是一个为 IncludeOS 设计的配置语言
                         「 Not Another Configuration Language」
                         => 把 nacl.txt 放到你 Service 的配置里面，在构建时你配置的
                         防火墙规则等就会变成 C++ 代码
                         主要用于 Iface，Filter，Gateway，Load_balancer 等网络功能配置
                         基于 ANTLR，Mustache；全部功能见目录下 cpp_template.mustache 足矣
|-- README.md
|-- Vagrantfile          Vagrant 是一个虚拟机管理器（提供类似 Docker 的体验）
|-- analyze.sh           调用 clang-tidy 进行源码检查（检查常见的静态错误）
                         INC="-Iapi/posix -Isrc/include -I$INCLUDEOS_PREFIX/includeos/x86_64/include/libcxx -I$INCLUDEOS_PREFIX/includeos/x86_64/include/newlib -Iapi -Imod -Imod/GSL -Imod/rapidjson/include -Imod/uzlib/src"
                         这个比较有用
|-- api                  include 目录之一，下面有详细介绍
|-- build_i686           build 目录之一，在 .gitignore 中忽略，下有介绍
|-- build_x86_64         build 目录之二，在 .gitignore 中忽略，下有介绍
|-- cmake                各种外部库和 pre / post install CMake 配置
|-- diskimagebuild       diskimagebuilder
                         作用：Creates a minimal read-only FAT file image from given source folder
                         需要 Guideline Support Library (mod/GLS)
|-- doc                  下面有 NOTES.md，描述 IncludeOS Design Choices（一些关于 Dev 类接口设计的讨论）
                         还有 Doxyfile，Doxygen 的配置文件，不过没什么文档..
|-- etc                  各种配置 shell 脚本，详情见下
|-- examples             一些示例工程，详情见下
|-- format_sources.sh    根据当前目录下的 .clang_format 搜索并格式化所有 .hpp 和 .cpp 代码
|-- install.sh           安装 IncludeOS 环境的脚本，详情见下
|-- lib                  一些库
| |-- LiveUpdate         IncludeOS 热更新库
| |-- mana               IncludeOS C++ Web 应用框架
| |-- mender             由 mender.io 提供的 IoT 设备 OTA 升级平台，在 IncludeOS 上的实现 
| |-- microLB            micro Load Balancer
| |-- protobuf           Protocol Buffers - Google's data interchange format
| `-- uplink             不知道是啥，一个 IncludeOS 的网络库
|-- linux                Userspace Linux platform
                         就是一个 IncludeOS 在 Linux 下的 Daemon，可以将为 IncludeOS 编写的程序
                         直接链接此库，作为 native 的 Linux 进程执行。
                         可选依赖为 Botan 等
|-- manual_build.sh      手动构建脚本 (pushd 是什么？)
|-- mod                  **Modules in Progress**
| |-- CMakeLists.txt
| |-- GSL                Guideline Support Library
                         
| |-- README.md          "This folder contains modules we're in the progress of porting. They will                          probably be moved to separate repositories."

| |-- http-parser        This is a parser for HTTP messages written in C. It parses both requests                           and responses. The parser is designed to be used in performance HTTP
                         applications. It does not make any syscalls nor allocations, it does not
                         buffer data, it can be interrupted at anytime. Depending on your
                         architecture, it only requires about 40 bytes of data per message
                         stream (in a web server that is per connection).

| |-- rapidjson          A fast JSON parser/generator for C++ with both SAX/DOM style API
                         By Tencent
| `-- uzlib              一个小号的 zlib，用来压缩/解压缩 deflate
|-- seed                 library（最终构建成 *.a） 和 service(最终构建成 ELF Multiboot Binary) 的模板
|-- src                  源码目录，下面介绍
|-- test                 单元测试（CASE + EXPECT）和整体测试（一个 service 的形式）
| |-- CMakeLists.txt     
| |-- README.md
| |-- fs
| |-- hw
| |-- kernel
| |-- lest               https://github.com/martinmoene/lest
                         所有单元测试用的测试框架
                         「A modern, C++11-native, single-file header-only, tiny framework for unit-tests」
| |-- lest_demo_test.cpp
| |-- lest_util
| |-- lib
| |-- linux
| |-- memdisk.fat
| |-- misc
| |-- mod
| |-- net
| |-- performance
| |-- plugin
| |-- posix
| |-- skipped_tests.json
| |-- stl
| |-- stress
| |-- testrunner.py
| |-- util
| `-- validate_tests.py
|-- test.sh              进行 IncludeOS_Minimal 的构建和运行
|-- test_ukvm.sh         Solo5 / ukvm 框架
                         Solo5 是一个「sandboxed execution environment for unikernels」
                         就是比虚拟机更轻量的「中间件」，夹在 Unikernel 和 Host OS 之间
                         可以不用
|-- vmbuild              主要用于根据 bootloader 和生成的 Multiboot Binary 生成启动的镜像
                         是个 C++ 项目
`-- vmrunner             主要用于启动虚拟机；负责各种虚拟机软件的选项，并且根据 vm.json 的结果
                         动态选择是否开启某些选项

```

## 外部项目
- Botan，一个密码学和 TLS 的现代 C++ 库。在 `cmake/botan.cmake` 有安装的 CMake 配置。


## `src` Hierarchy
- src
|-- CMakeLists.txt       
|-- arch
| |-- i686
| `-- x86_64
|-- chainload
| |-- CMakeLists.txt
| |-- build
| |-- hotswap.cpp
| `-- service.cpp
|-- crt
| |-- c_abi.c
| |-- crti.asm
| |-- crtn.asm
| |-- ctype_b_loc.c
| |-- ctype_tolower_loc.c
| |-- cxx_abi.cpp
| |-- pthread.c
| |-- quick_exit.cpp
| |-- string.c
| `-- string.h
|-- drivers
| |-- CMakeLists.txt
| |-- disk_logger.cpp
| |-- disk_logger.hpp
| |-- disklog_reader.cpp
| |-- e1000.cpp
| |-- e1000.hpp
| |-- e1000_defs.hpp
| |-- heap_debugging.cpp
| |-- ide.cpp
| |-- ide.hpp
| |-- ip4_reassembly.cpp
| |-- solo5blk.cpp
| |-- solo5blk.hpp
| |-- solo5net.cpp
| |-- solo5net.hpp
| |-- stdout
| |-- vga_emergency.cpp
| |-- virtioblk.cpp
| |-- virtioblk.hpp
| |-- virtiocon.cpp
| |-- virtiocon.hpp
| |-- virtionet.cpp
| |-- virtionet.hpp
| |-- vmxnet3.cpp
| |-- vmxnet3.hpp
| `-- vmxnet3_queues.hpp
|-- fs
| |-- dirent.cpp
| |-- disk.cpp
| |-- fat.cpp
| |-- fat_async.cpp
| |-- fat_sync.cpp
| |-- filesystem.cpp
| |-- mbr.cpp
| |-- memdisk.cpp
| `-- path.cpp
|-- hw
| |-- msi.cpp
| |-- nic.cpp
| |-- pci_device.cpp
| |-- pci_msi.cpp
| |-- ps2.cpp
| |-- serial.cpp
| `-- vga_gfx.cpp
|-- include
| `-- kprint
|-- kernel
| |-- block.cpp
| |-- context.cpp
| |-- context_asm.asm
| |-- cpuid.cpp
| |-- elf.cpp
| |-- events.cpp
| |-- fiber.cpp
| |-- heap.cpp
| |-- memmap.cpp
| |-- multiboot.cpp
| |-- os.cpp
| |-- pci_manager.cpp
| |-- profile.cpp
| |-- rdrand.cpp
| |-- rng.cpp
| |-- rtc.cpp
| |-- scoped_profiler.cpp
| |-- service_stub.cpp
| |-- solo5_manager.cpp
| |-- syscalls.cpp
| |-- system_log.cpp
| |-- terminal.cpp
| |-- timers.cpp
| |-- tls.cpp
| `-- vga.cpp
|-- memdisk
| |-- empty.asm
| |-- memdisk.asm
| `-- memdisk.py
|-- musl
| |-- CMakeLists.txt
| |-- _lseek.cpp
| |-- access.cpp
| |-- brk.cpp
| |-- chmod.cpp
| |-- chown.cpp
| |-- clock_gettime.cpp
| |-- close.cpp
| |-- common.hpp
| |-- creat.cpp
| |-- cwd.cpp
| |-- dup3.cpp
| |-- execve.cpp
| |-- exit.cpp
| |-- fchmod.cpp
| |-- fchmodat.cpp
| |-- fchown.cpp
| |-- fcntl.cpp
| |-- fstat.cpp
| |-- fstatat.cpp
| |-- fsync.cpp
| |-- ftruncate.cpp
| |-- futex.cpp
| |-- getdents.cpp
| |-- geteuid.cpp
| |-- getgid.cpp
| |-- getpid.cpp
| |-- getrlimit.cpp
| |-- gettid.cpp
| |-- gettimeofday.cpp
| |-- getuid.cpp
| |-- ioctl.cpp
| |-- kill.cpp
| |-- lseek.cpp
| |-- madvise.cpp
| |-- mincore.cpp
| |-- mkdir.cpp
| |-- mkdirat.cpp
| |-- mknod.cpp
| |-- mknodat.cpp
| |-- mmap.cpp
| |-- mremap.cpp
| |-- msync.cpp
| |-- munmap.cpp
| |-- nanosleep.cpp
| |-- open.cpp
| |-- openat.cpp
| |-- pipe.cpp
| |-- poll.cpp
| |-- prlimit64.cpp
| |-- read.cpp
| |-- readlink.cpp
| |-- readv.cpp
| |-- rename.cpp
| |-- rmdir.cpp
| |-- rt_sigaction.cpp
| |-- sched_getaffinity.cpp
| |-- sched_yield.cpp
| |-- select.cpp
| |-- set_robust_list.cpp
| |-- set_tid_address.cpp
| |-- setgid.cpp
| |-- setpgid.cpp
| |-- setrlimit.cpp
| |-- setsid.cpp
| |-- setuid.cpp
| |-- sigmask.cpp
| |-- socketcall.cpp
| |-- stat.cpp
| |-- stub.hpp
| |-- sync.cpp
| |-- syscall_n.cpp
| |-- sysinfo.cpp
| |-- umask.cpp
| |-- uname.cpp
| |-- unlink.cpp
| |-- utimensat.cpp
| |-- wait4.cpp
| |-- write.cpp
| `-- writev.cpp
|-- net
| |-- buffer_store.cpp
| |-- checksum.cpp
| |-- configure.cpp
| |-- conntrack.cpp
| |-- dhcp
| |-- dns
| |-- ethernet
| |-- http
| |-- https
| |-- inet.cpp
| |-- ip4
| |-- ip6
| |-- nat
| |-- openssl
| |-- packet_debug.cpp
| |-- super_stack.cpp
| |-- tcp
| |-- vlan_manager.cpp
| `-- ws
|-- platform
| |-- kvm
| |-- x86_nano
| |-- x86_pc
| `-- x86_solo5
|-- plugins
| |-- CMakeLists.txt
| |-- autoconf.cpp
| |-- example.cpp
| |-- field_medic
| |-- nacl.cpp
| |-- syslog.cpp
| |-- syslogd.cpp
| |-- system_log.cpp
| |-- terminal.cpp
| |-- unik.cpp
| `-- vfs.cpp
|-- posix
| |-- fd.cpp
| |-- file_fd.cpp
| |-- memalign.cpp
| |-- pthread.cpp
| |-- stdlib.cpp
| |-- sys
| |-- tcp_fd.cpp
| |-- udp_fd.cpp
| |-- unistd.cpp
| `-- unix_fd.cpp
|-- service_name.cpp
|-- util
| |-- async.cpp
| |-- autoconf.cpp
| |-- config.cpp
| |-- crc32.cpp
| |-- logger.cpp
| |-- memstream.c
| |-- path_to_regex.cpp
| |-- percent_encoding.cpp
| |-- pmr_default.cpp
| |-- sha1.cpp
| |-- statman.cpp
| |-- statman_liu.cpp
| |-- syslog_facility.cpp
| |-- syslogd.cpp
| |-- tar.cpp
| `-- uri.cpp
|-- version.cpp
`-- virtio
    |-- virtio.cpp
    `-- virtio_queue.cpp