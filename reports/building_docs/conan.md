# Conan 学习记录

因为新的构建系统用了 Conan，所以整理一二，作于下文。

Conan 是一个 C++ 项目构建工具，基于 Python。

文档：(https://docs.conan.io/)[https://docs.conan.io/]
## Conan 想干什么
如果不知道 Conan 想干什么，下面的内容就会很困惑。

我们知道，对于一个很大的项目，会有很多的库需要依赖。之前 IncludeOS 采取的方式是自己写很多相关依赖库的安装和配置脚本（在 `/cmake` 文件夹下面） 

大体来说，Conan 想实现下面的目标：
- 自动安装需要的库（通过把它们打包，并且用包之间的依赖关系）
- 类似 `docker` 一样，可以把自己的包（Recipe / Recipe + Source）放到 `BinTray` / `BinTray Community Server` 上面；只要用 `conda install` 等就可以从指定的 remote server 下载
- 跨平台，并且支持多个构建工具
  - 在 Linux 上我们经常用 （CMake -> make）这两个工具；在 Win 上我们经常用微软的啥啥构建工具
  - Conan 都应该支持，这样就必须要可以生成用于多种构建系统的配置文件，由他们进行最终的编译工作（比如 `CMakeLists.txt`）

## Conan 安装
- `pip install conan` # **建议使用 `virtualenv`**，否则可能把 Python 依赖库搞得一团糟

在 Arch 上的 AUR 源有 `conda` 包，用 `yay` 这种可以安装；手工安装也可以，但是它还依赖三个 AUR 的包，所以自己安装会比较麻烦。

## Conan 是如何管理包的

### 如何记录包的信息

每一个包都有 `conanfile.py`（称为 `Recipe`），描述包括但不限于如下信息：
- name, version, license 等 metadata
- 可以接受的 `generators` （下面介绍）
- source() => 用来描述在哪里找到源代码（如果源代码和 `condafile.py` 在一个地方，就可以省略）
- build() => 用来描述如何构建的函数，比如设置一些 CMake 设置什么的
- 其他「Action」

每个包都可以描述自己的依赖，依赖信息在类的 `self.requires` 里面，并且可以灵活配置；在 `build_requires` 里面也可以写。

Conan 还有几种类别的设置选项：
- Settings 是全局（platform-specific）的设置，用来表述系统的环境，有四种类型 `"os","arch","build_type","compiler"`。
  在每个包的 `conanfile.py` 里面可以设置 `settings` 这个成员（**请注意，这个和上面说的 类型 是一回事，但是和具体的设置不是一回事**）。注意，这不是「默认值」，而是表示当上面的**类型**改变的时候，包就必须重新编译。
- Options 是包内的设置，比如哪个文件应该静态链接，是在包里可以有默认值的。
- Environment Variables 指定在编译的时候被设置的环境变量值。
  - 如果希望指定交叉编译器，那显然得设置一下 `CC` 和 `CXX` 的值；用命令行可以指定（`-E` option）
- Build Requires 来描述需要的依赖
- Generators 用来描述要生成的构建系统的文件（比如 `cmake` 就会生成一坨 cmake 的东西）

> 用 `-s var=val` 这种格式的命令，可以更改选项的值。
- 比如 `conan install /home/libreliu/OS/IncludeOS-dev/conanfile.py -s build_type=Release -s compiler=gcc -s compiler.version=8 -s compiler.libcxx=libstdc++11 -g=cmake` 中的 `-s` 选项就指定了全局的 Settings 设置

> 如果没有指定设置的话，Conan 会从 `~/.conan/profiles/default` 来读取默认的 `settings` `options` `environment variables` `build_requires`。
> `~/.conan/profiles/` 还有其它的 profile，用 `-pr absolute_or_relative_path` 就可以。
> 比如 `conan create . demo/testing -pr=gcc-8.2.0-linux-aarch64-toolchain`

编译一个依赖项（或者说找一个依赖项，编译到 local cache 里面）用 `conan install .`（`.` 代表当前路径的 `conanfile.py`；手工指定 `conanfile.py` 也可以，比如 `conan install aaa/conanfile.py`）

### 一个用 conan 管理包的例子
（其实对 IncludeOS 理解本身没啥帮助，但是记录一下，对整个流程有个完整的理解）

写一个用 Poco 库的 MD5 计算器：（代码就是这么简单）

```c
 #include "Poco/MD5Engine.h"
 #include "Poco/DigestStream.h"

 #include <iostream>


 int main(int argc, char** argv)
 {
     Poco::MD5Engine md5;
     Poco::DigestOutputStream ds(md5);
     ds << "abcdefghijklmnopqrstuvwxyz";
     ds.close();
     std::cout << Poco::DigestEngine::digestToHex(md5.digest()) << std::endl;
     return 0;
 }
```

1. 找一下 conan-center 这个远程服务器里面已经有的 Poco 库的 Recipe（也就是包）：
```bash
$ conan search Poco* --remote=conan-center
Existing package recipes:

Poco/1.7.8p3@pocoproject/stable
Poco/1.7.9@pocoproject/stable
Poco/1.7.9p1@pocoproject/stable
Poco/1.7.9p2@pocoproject/stable
Poco/1.8.0.1@pocoproject/stable
Poco/1.8.0@pocoproject/stable
Poco/1.8.1@pocoproject/stable
Poco/1.9.0@pocoproject/stable
```
然后我们对 1.9.0 感兴趣，要看一下：
```bash
$ conan inspect Poco/1.9.0@pocoproject/stable
...
name: Poco
version: 1.9.0
url: http://github.com/pocoproject/conan-poco
license: The Boost Software License 1.0
author: None
description: Modern, powerful open source C++ class libraries for building network- and internet-based applications that run on desktop, server, mobile and embedded systems.
generators: ('cmake', 'txt')
exports: None
exports_sources: ('CMakeLists.txt', 'PocoMacros.cmake')
short_paths: False
apply_env: True
build_policy: None
settings: ('os', 'arch', 'compiler', 'build_type')
options:
    enable_apacheconnector: [True, False]
    shared: [True, False]
default_options:
    enable_apacheconnector: False
    shared: False
```
2. 写一个 `conanfile.txt`，包含如下内容：
```
 [requires]
 Poco/1.9.0@pocoproject/stable

 [generators]
 cmake
```
这个里面描述了依赖包和依赖的后端构建工具。

3. 运行下面的东西：
```
$ mkdir build && cd build
$ conan install ..
...
Requirements
    OpenSSL/1.0.2o@conan/stable from 'conan-center' - Downloaded
    Poco/1.9.0@pocoproject/stable from 'conan-center' - Cache
    zlib/1.2.11@conan/stable from 'conan-center' - Downloaded
Packages
    OpenSSL/1.0.2o@conan/stable:606fdb601e335c2001bdf31d478826b644747077 - Download
    Poco/1.9.0@pocoproject/stable:09378ed7f51185386e9f04b212b79fe2d12d005c - Download
    zlib/1.2.11@conan/stable:6cc50b139b9c3d27b3e9042d5f5372d327b3a9f7 - Download

zlib/1.2.11@conan/stable: Retrieving package 6cc50b139b9c3d27b3e9042d5f5372d327b3a9f7 from remote 'conan-center'
...
Downloading conan_package.tgz
[==================================================] 99.8KB/99.8KB
...
zlib/1.2.11@conan/stable: Package installed 6cc50b139b9c3d27b3e9042d5f5372d327b3a9f7
OpenSSL/1.0.2o@conan/stable: Retrieving package 606fdb601e335c2001bdf31d478826b644747077 from remote 'conan-center'
...
Downloading conan_package.tgz
[==================================================] 5.5MB/5.5MB
...
OpenSSL/1.0.2o@conan/stable: Package installed 606fdb601e335c2001bdf31d478826b644747077
Poco/1.9.0@pocoproject/stable: Retrieving package 09378ed7f51185386e9f04b212b79fe2d12d005c from remote 'conan-center'
...
Downloading conan_package.tgz
[==================================================] 11.5MB/11.5MB
...
Poco/1.9.0@pocoproject/stable: Package installed 09378ed7f51185386e9f04b212b79fe2d12d005c
PROJECT: Generator cmake created conanbuildinfo.cmake
PROJECT: Generator txt created conanbuildinfo.txt
PROJECT: Generated conaninfo.txt
```
> Conan installed our Poco dependency but also the transitive dependencies for it: OpenSSL and zlib. It has also generated a conanbuildinfo.cmake file for our build system.
4. **自己写的**`CMakeLists.txt` 里面加上 `conanbuildinfo.cmake` 这个自动生成文件的引用，和 `conan_basic_setup()` 函数。
```
 cmake_minimum_required(VERSION 2.8.12)
 project(MD5Encrypter)

 add_definitions("-std=c++11")

 include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
 conan_basic_setup()

 add_executable(md5 md5.cpp)
 target_link_libraries(md5 ${CONAN_LIBS})
```
5. 编译运行：
```shell
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
./bin/md5 # 运行
```

## 查看包之间的依赖关系
[Here @ conan.io](https://docs.conan.io/en/latest/getting_started.html#inspecting-dependencies)

这里描述了创建依赖图、查看全部项目依赖包 metadata 等小技巧。

## Conan 的 local cache
（See `~/.conan/data`）

Conan 有远程服务器，也有本地的缓存。在检索依赖项的时候，如果有依赖在本地已经存在，就会采用本地的版本。

如果本地的编译版本不符合 os, compiler 等要求，就会重新编译/去远程仓库找有没有对应设置的 prebuilt binary。

在本地 local cache 打好的包也可以 Upload 到云端（还没看）

IncludeOS 的 `README.md` 也提到了这一点（ediable 啥的）。

## Conan: create a package
The conan create command does the following:

- Copies (“export” in Conan terms) the conanfile.py from the user folder into the local cache.
- Installs the package, forcing it to be built from the sources.
- Moves to the test_package folder and creates a temporary build folder.
- Executes the conan install .., to install the requirements of the test_package/conanfile.py. Note that it will build “Hello” from the sources.
- Builds and launches the example consuming application, calling the test_package/conanfile.py build() and test() methods respectively.

Using Conan commands, the conan create command would be equivalent to:

```
$ conan export . demo/testing
$ conan install Hello/0.1@demo/testing --build=Hello
# package is created now, use test to test it
$ conan test test_package Hello/0.1@demo/testing
```

The conan create command receives the same command line parameters as conan install so you can pass to it the same settings, options, and command line switches. If you want to create and test packages for different configurations, you could:

```
$ conan create . demo/testing -s build_type=Debug
$ conan create . demo/testing -o Hello:shared=True -s arch=x86
$ conan create . demo/testing -pr my_gcc49_debug_profile
...
$ conan create ...
```

> Conan: build from external source
> - The source() method will be called after the checkout process, so you can still use it to patch something or retrieve more sources, but it is not necessary in most cases.

## Traditional Build & Run

In the previous examples, we used the conan create command to create a package of our library. Every time it is run, Conan performs the following costly operations:

Copy the sources to a new and clean build folder.
Build the entire library from scratch.
Package the library once it is built.
Build the test_package example and test if it works.



## 我的编译过程记录
- `git clone https://github.com/IncludeOS/IncludeOS.git -b dev`
- `cd IncludeOS && make ../deployed_src`
- `conan source . --source-folder=../deployed_src`
- `cd .. && cp -r deployed_src/ modified_src/ && cd modified_src`
- 打开 `conanfile.py`，编辑掉 `if not ... nano` 下面的 `require` 项
- `conan install . --install-folder=../mod_src_build -pr gcc-8.2.0-linux-aarch64 -e CC=aarch64-linux-gnu-gcc -e CXX=aarch64-linux-gnu-g++`
- 现在在`../mod_src_build`目录下有了 `activate.sh`，内容如下：
```
OLD_PS1="$PS1"
export OLD_PS1
PS1="(conanenv) $PS1"
export PS1
CC="aarch64-linux-gnu-gcc"
export CC
CXX="aarch64-linux-gnu-g++"
export CXX
CFLAGS="-mcpu=cortex-a53 -O2 -g"
export CFLAGS
CXXFLAGS="-mcpu=cortex-a53 -O2 -g"
export CXXFLAGS
PATH="/home/libreliu/.conan/data/binutils/2.31/includeos/toolchain/package/1907037da8323f14f8235c9a3fabcb665a84c867/aarch64-elf/bin":"/home/libreliu/.conan/data/binutils/2.31/includeos/toolchain/package/1907037da8323f14f8235c9a3fabcb665a84c867/bin"${PATH+:$PATH}
export PATH
```
> `activate.sh`是如何生成的？为什么生成了 `conaninfo.txt` ？
- `conan build . -bf ../mod_src_build` （在 `modified_src` 下运行）
```
conanfile.py (includeos/0.0.0@None/None): Running build()
-- The C compiler identification is GNU 8.3.0
-- The CXX compiler identification is GNU 8.3.0
-- Check for working C compiler: /usr/bin/aarch64-linux-gnu-gcc
-- Check for working C compiler: /usr/bin/aarch64-linux-gnu-gcc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/aarch64-linux-gnu-g++
-- Check for working CXX compiler: /usr/bin/aarch64-linux-gnu-g++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Conan: called by CMake conan helper
-- Conan: Adjusting output directories
-- Conan: Using cmake global configuration
-- Conan: Adjusting default RPATHs Conan policies
-- Conan: Adjusting language standard
-- Conan: Compiler GCC>=5, checking major version 8
-- Conan: Checking correct version: 8
-- Target CPU aarch64
-- Target triple aarch64-pc-linux-elf
-- The ASM compiler identification is GNU
-- Found assembler: /usr/bin/aarch64-linux-gnu-gcc
-- Configuring done
-- Generating done
CMake Warning:
  Manually-specified variables were not used by the project:

    CMAKE_EXPORT_NO_PACKAGE_REGISTRY
    CMAKE_INSTALL_BINDIR
    CMAKE_INSTALL_DATAROOTDIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
    CMAKE_INSTALL_LIBEXECDIR
    CMAKE_INSTALL_OLDINCLUDEDIR
    CMAKE_INSTALL_SBINDIR


-- Build files have been written to: /home/libreliu/OS/IncludeOS-dev-new/mod_src_build
Scanning dependencies of target hal
Scanning dependencies of target util
Scanning dependencies of target kernel
Scanning dependencies of target crt
[  1%] Building CXX object src/hal/CMakeFiles/hal.dir/machine.cpp.o
[  2%] Building C object src/crt/CMakeFiles/crt.dir/c_abi.c.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/crt/c_abi.c: In function '__vsnprintf_chk':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/crt/c_abi.c:111:38: warning: unused parameter 'maxlen' [-Wunused-parameter]
 int __vsnprintf_chk (char *s, size_t maxlen, int flags, size_t slen,
                               ~~~~~~~^~~~~~
[  3%] Building CXX object src/util/CMakeFiles/util.dir/async.cpp.o
[  4%] Building CXX object src/kernel/CMakeFiles/kernel.dir/block.cpp.o
[  4%] Building C object src/crt/CMakeFiles/crt.dir/ctype_b_loc.c.o
[  5%] Building C object src/crt/CMakeFiles/crt.dir/ctype_tolower_loc.c.o
[  5%] Building C object src/crt/CMakeFiles/crt.dir/string.c.o
[  5%] Building CXX object src/crt/CMakeFiles/crt.dir/quick_exit.cpp.o
[  6%] Building CXX object src/crt/CMakeFiles/crt.dir/cxx_abi.cpp.o
[  6%] Building CXX object src/kernel/CMakeFiles/kernel.dir/cpuid.cpp.o
[  6%] Built target hal
[  6%] Building CXX object src/util/CMakeFiles/util.dir/statman.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/async.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/async:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/async.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
[  6%] Building CXX object src/kernel/CMakeFiles/kernel.dir/elf.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/async.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/async:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/async.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/elf.cpp: In function 'void elf_check_symbols_ok()':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/elf.cpp:459:31: warning: array subscript 0 is above array bounds of 'ElfSym [0]' {aka 'Elf64_Sym [0]'} [-Warray-bounds]
   uint32_t csum_strs = crc32c(&hdr->syms[hdr->symtab_entries], hdr->strtab_size);
                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
[  7%] Building CXX object src/kernel/CMakeFiles/kernel.dir/events.cpp.o
[  7%] Building CXX object src/kernel/CMakeFiles/kernel.dir/fiber.cpp.o
[  7%] Built target crt
Scanning dependencies of target net
Scanning dependencies of target fs
[  7%] Building CXX object src/kernel/CMakeFiles/kernel.dir/memmap.cpp.o
[  7%] Building CXX object src/fs/CMakeFiles/fs.dir/disk.cpp.o
[  7%] Building CXX object src/net/CMakeFiles/net.dir/checksum.cpp.o
[  7%] Building CXX object src/util/CMakeFiles/util.dir/logger.cpp.o
[  7%] Building CXX object src/net/CMakeFiles/net.dir/buffer_store.cpp.o
[  8%] Building CXX object src/fs/CMakeFiles/fs.dir/filesystem.cpp.o
[  9%] Building CXX object src/kernel/CMakeFiles/kernel.dir/multiboot.cpp.o
[ 10%] Building CXX object src/util/CMakeFiles/util.dir/sha1.cpp.o
[ 11%] Building CXX object src/net/CMakeFiles/net.dir/inet.cpp.o
[ 11%] Building CXX object src/util/CMakeFiles/util.dir/syslog_facility.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp: In function 'uintptr_t _multiboot_free_begin(uintptr_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:76:9: warning: format '%x' expects argument of type 'unsigned int', but argument 2 has type 'multiboot_info*' [-Wformat=]
   debug("* Multiboot begin: 0x%x \n", info);
         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~  ~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:28:31: note: in definition of macro 'debug'
 #define debug(X,...)  kprintf(X,##__VA_ARGS__);
                               ^
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:80:76: warning: cast to pointer from integer of different size [-Wint-to-pointer-cast]
     debug("* Multiboot cmdline @ 0x%x: %s \n", info->cmdline, (char*)info->cmdline);
                                                                            ^~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:28:35: note: in definition of macro 'debug'
 #define debug(X,...)  kprintf(X,##__VA_ARGS__);
                                   ^~~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:92:9: warning: format '%x' expects argument of type 'unsigned int', but argument 2 has type 'uintptr_t' {aka 'long unsigned int'} [-Wformat=]
   debug("* Multiboot end: 0x%x \n", multi_end);
         ^~~~~~~~~~~~~~~~~~~~~~~~~~  ~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:28:31: note: in definition of macro 'debug'
 #define debug(X,...)  kprintf(X,##__VA_ARGS__);
                               ^
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:112:9: warning: format '%x' expects argument of type 'unsigned int', but argument 2 has type 'uintptr_t' {aka 'long unsigned int'} [-Wformat=]
   debug("* Multiboot end: 0x%x \n", multi_end);
         ^~~~~~~~~~~~~~~~~~~~~~~~~~  ~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/multiboot.cpp:28:31: note: in definition of macro 'debug'
 #define debug(X,...)  kprintf(X,##__VA_ARGS__);
                               ^
[ 11%] Building CXX object src/fs/CMakeFiles/fs.dir/dirent.cpp.o
[ 11%] Building CXX object src/kernel/CMakeFiles/kernel.dir/pci_manager.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/inet.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/inet.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
[ 11%] Building CXX object src/fs/CMakeFiles/fs.dir/mbr.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces:20,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/syslog_facility.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
[ 12%] Building CXX object src/fs/CMakeFiles/fs.dir/path.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces:20,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/syslog_facility.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
[ 12%] Building CXX object src/fs/CMakeFiles/fs.dir/fat.cpp.o
[ 12%] Building CXX object src/kernel/CMakeFiles/kernel.dir/os.cpp.o
[ 13%] Building CXX object src/kernel/CMakeFiles/kernel.dir/profile.cpp.o
[ 13%] Building CXX object src/util/CMakeFiles/util.dir/syslogd.cpp.o
[ 13%] Building CXX object src/fs/CMakeFiles/fs.dir/fat_async.cpp.o
[ 13%] Building CXX object src/kernel/CMakeFiles/kernel.dir/syscalls.cpp.o
[ 13%] Building CXX object src/net/CMakeFiles/net.dir/interfaces.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/syscalls.cpp:214:2: warning: #warning "panic() handler not implemented for selected arch" [-Wcpp]
 #warning "panic() handler not implemented for selected arch"
  ^~~~~~~
[ 14%] Building CXX object src/kernel/CMakeFiles/kernel.dir/service_stub.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/interfaces.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/interfaces.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/interfaces.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
[ 14%] Building CXX object src/kernel/CMakeFiles/kernel.dir/terminal.cpp.o
[ 15%] Building CXX object src/fs/CMakeFiles/fs.dir/fat_sync.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/terminal.cpp:20:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/terminal.cpp:20:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
[ 16%] Building CXX object src/util/CMakeFiles/util.dir/percent_encoding.cpp.o
[ 16%] Building CXX object src/fs/CMakeFiles/fs.dir/memdisk.cpp.o
[ 16%] Building CXX object src/util/CMakeFiles/util.dir/path_to_regex.cpp.o
[ 16%] Built target fs
Scanning dependencies of target posix
[ 17%] Building CXX object src/posix/CMakeFiles/posix.dir/fd.cpp.o
[ 17%] Building CXX object src/posix/CMakeFiles/posix.dir/file_fd.cpp.o
[ 17%] Building CXX object src/kernel/CMakeFiles/kernel.dir/timers.cpp.o
[ 17%] Building CXX object src/net/CMakeFiles/net.dir/packet_debug.cpp.o
[ 17%] Building CXX object src/posix/CMakeFiles/posix.dir/tcp_fd.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/packet_debug.cpp: In function 'void net::print_last_packet()':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/packet_debug.cpp:51:21: warning: format '%hx' expects argument of type 'int', but argument 3 has type 'net::Ethertype' [-Wformat=]
     fprintf(stderr, "Ethernet type: 0x%hx ", eth->type());
                     ^~~~~~~~~~~~~~~~~~~~~~~  ~~~~~~~~~~~
[ 18%] Building CXX object src/net/CMakeFiles/net.dir/conntrack.cpp.o
[ 19%] Building CXX object src/kernel/CMakeFiles/kernel.dir/rng.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/tcp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/tcp_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/tcp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/tcp_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
[ 19%] Building CXX object src/kernel/CMakeFiles/kernel.dir/tls.cpp.o
[ 19%] Building CXX object src/kernel/CMakeFiles/kernel.dir/vga.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/tcp_fd.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 20%] Building CXX object src/kernel/CMakeFiles/kernel.dir/context.cpp.o
[ 20%] Building CXX object src/kernel/CMakeFiles/kernel.dir/heap.cpp.o
[ 20%] Building CXX object src/net/CMakeFiles/net.dir/vlan_manager.cpp.o
[ 20%] Building CXX object src/kernel/CMakeFiles/kernel.dir/kernel.cpp.o
[ 21%] Building CXX object src/posix/CMakeFiles/posix.dir/udp_fd.cpp.o
[ 22%] Building CXX object src/util/CMakeFiles/util.dir/crc32.cpp.o
[ 23%] Building CXX object src/kernel/CMakeFiles/kernel.dir/liveupdate.cpp.o
[ 23%] Building C object src/util/CMakeFiles/util.dir/memstream.c.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/memstream.c:19:10: fatal error: x86intrin.h: No such file or directory
 #include <x86intrin.h>
          ^~~~~~~~~~~~~
compilation terminated.
make[2]: *** [src/util/CMakeFiles/util.dir/build.make:180：src/util/CMakeFiles/util.dir/memstream.c.o] 错误 1
make[1]: *** [CMakeFiles/Makefile2:305：src/util/CMakeFiles/util.dir/all] 错误 2
make[1]: *** 正在等待未完成的任务....
[ 23%] Building CXX object src/kernel/CMakeFiles/kernel.dir/rtc.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/udp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/udp_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
[ 23%] Building CXX object src/kernel/CMakeFiles/kernel.dir/system_log.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/udp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/udp_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/liveupdate.cpp: In function 'void kernel::setup_liveupdate()':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/kernel/liveupdate.cpp:23:16: warning: unused variable 'size' [-Wunused-variable]
   const size_t size = kernel::state().liveupdate_size;
                ^~~~
[ 23%] Building CXX object src/posix/CMakeFiles/posix.dir/unix_fd.cpp.o
[ 23%] Built target kernel
[ 23%] Building CXX object src/net/CMakeFiles/net.dir/addr.cpp.o
[ 24%] Building CXX object src/net/CMakeFiles/net.dir/ws/websocket.cpp.o
[ 24%] Building CXX object src/net/CMakeFiles/net.dir/configure.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/configure.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/configure.hpp:30:10: fatal error: rapidjson/document.h: No such file or directory
 #include <rapidjson/document.h>
          ^~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
make[2]: *** [src/net/CMakeFiles/net.dir/build.make:180：src/net/CMakeFiles/net.dir/configure.cpp.o] 错误 1
make[2]: *** 正在等待未完成的任务....
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/unix_fd.hpp:21,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/unix_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/unix_fd.hpp:21,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/posix/unix_fd.cpp:17:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/stream.hpp:1,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/http/connection.hpp:26,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/http/server_connection.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/http/server.hpp:24,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ws/websocket.hpp:24,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/ws/websocket.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/http/basic_client.hpp:26,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ws/websocket.hpp:25,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/net/ws/websocket.cpp:18:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp: At global scope:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
[ 24%] Built target posix
make[1]: *** [CMakeFiles/Makefile2:360：src/net/CMakeFiles/net.dir/all] 错误 2
make: *** [Makefile:130：all] 错误 2
ERROR: conanfile.py (includeos/0.0.0@None/None): Error in build() method, line 87
	cmake.build()
	ConanException: Error 512 while executing cmake --build '/home/libreliu/OS/IncludeOS-dev-new/mod_src_build' '--' '-j4'
[libreliu@thinkpad-ssd modified_src]$ 
```

## What is the relation?
Conan 负责外部库的 includePath 等，以及静态库构建

IncludeOS 自己的构建则用 CMake 本身解决！

参见 `src/CMakeLists.txt`，通过先定义「LIBARIES」变量，然后用`add_subdirectory()`调用，进而调用里面的子的 `CMakeLists.txt` 这样子。

所以现在 Conan 已经基本不用动了。每次改的话，用 conan export 弄到 local cache 然后 Conan build 就行吧..

在 `dev` 分支下的 `README.md` 有这种描述：
```
We are now ready to build the package. Assuming the build-folder is called build under the includeos source directory the following is enough.

$ cd [includeos source root]
$ conan install -if build . -pr <conan_profile> (-o options like platform=nano etc)
$ conan build -bf build .
After making changes to the code you can rebuild the package with

$ cd build && make
   or
$ cmake build --build
```
如果再运行一遍 CMake 的话，CMake 会重新解析那些依赖的 CMakeLists.txt，所以如果源码有改动（eg 增加了新的 `.c` **而要在对应的 `src` 的目录下改动 `CMakeLists.txt`**），就要重新`cmake ../mod_src_build/`，输出大概如下：
```
[libreliu@thinkpad-ssd modified_src]$ cmake ../mod_src_build/
-- Conan: called by CMake conan helper
-- Conan: Adjusting output directories
-- Conan: Using cmake global configuration
-- Conan: Adjusting default RPATHs Conan policies
-- Conan: Adjusting language standard
-- Conan: Compiler GCC>=5, checking major version 8
-- Conan: Checking correct version: 8
-- Target CPU aarch64
-- Target triple aarch64-pc-linux-elf
-- Configuring done
-- Generating done
-- Build files have been written to: /home/libreliu/OS/IncludeOS-dev-new/mod_src_build
```

然后再重新 `make`，就跟常规的 CMake 项目一致了。在我这里 `cd ../mod_src_build && make` 仍然会出 `src/util/memstream.c` 的错误：
```
[libreliu@thinkpad-ssd mod_src_build]$ make
[  3%] Built target crt
[  4%] Built target hal
[ 12%] Built target kernel
[ 12%] Building C object src/util/CMakeFiles/util.dir/memstream.c.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/util/memstream.c:19:10: fatal error: x86intrin.h: No such file or directory
 #include <x86intrin.h>
          ^~~~~~~~~~~~~
compilation terminated.
make[2]: *** [src/util/CMakeFiles/util.dir/build.make:180：src/util/CMakeFiles/util.dir/memstream.c.o] 错误 1
make[1]: *** [CMakeFiles/Makefile2:305：src/util/CMakeFiles/util.dir/all] 错误 2
make: *** [Makefile:130：all] 错误 2
[libreliu@thinkpad-ssd mod_src_build]$ 
```
这是显然的，因为我还没有把那些玩意注释掉（先把 `arch` 无关的代码端掉）


### after patching main cmake script
```
[libreliu@thinkpad-ssd IncludeOS-dev-new]$ cd modified_src/
[libreliu@thinkpad-ssd modified_src]$ cmake ../mod_src_build
-- Conan: called by CMake conan helper
-- Conan: Adjusting output directories
-- Conan: Using cmake global configuration
-- Conan: Adjusting default RPATHs Conan policies
-- Conan: Adjusting language standard
-- Conan: Compiler GCC>=5, checking major version 8
-- Conan: Checking correct version: 8
-- Target CPU aarch64
-- Target triple aarch64-pc-linux-elf
-- Configuring done
-- Generating done
-- Build files have been written to: /home/libreliu/OS/IncludeOS-dev-new/mod_src_build
[libreliu@thinkpad-ssd modified_src]$ cd ../mod_src_build/
[libreliu@thinkpad-ssd mod_src_build]$ make
[  5%] Built target crt
Scanning dependencies of target os
[  6%] Building CXX object src/CMakeFiles/os.dir/version.cpp.o
[  7%] Linking CXX static library ../lib/libos.a
[  7%] Built target os
Scanning dependencies of target arch
[  8%] Building CXX object src/arch/aarch64/CMakeFiles/arch.dir/paging.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/arch/aarch64/paging.cpp: In function 'os::mem::Map os::mem::map(os::mem::Map, const char*)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/arch/aarch64/paging.cpp:30:15: warning: unused parameter 'm' [-Wunused-parameter]
   Map map(Map m, const char* name) {
           ~~~~^
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/arch/aarch64/paging.cpp:30:30: warning: unused parameter 'name' [-Wunused-parameter]
   Map map(Map m, const char* name) {
                  ~~~~~~~~~~~~^~~~
[  9%] Building CXX object src/arch/aarch64/CMakeFiles/arch.dir/cpu.cpp.o
[ 10%] Building CXX object src/arch/aarch64/CMakeFiles/arch.dir/timer.cpp.o
[ 11%] Building CXX object src/arch/aarch64/CMakeFiles/arch.dir/syscall_entry.cpp.o
[ 12%] Building ASM object src/arch/aarch64/CMakeFiles/arch.dir/arch_start.asm.o
[ 13%] Building ASM object src/arch/aarch64/CMakeFiles/arch.dir/exceptions.asm.o
[ 14%] Linking CXX static library ../../../lib/libarch.a
[ 14%] Built target arch
Scanning dependencies of target aarch64_default
[ 14%] Building ASM object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/start_aarch64.asm.o
[ 15%] Building ASM object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/stop.asm.o
[ 16%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/serial1.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/serial.hpp:24,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/serial1.cpp:1:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'uint8_t hw::inb(uint16_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:26:38: warning: unused parameter 'port' [-Wunused-parameter]
   static inline uint8_t inb(uint16_t port)
                             ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'uint16_t hw::inw(uint16_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:38:39: warning: unused parameter 'port' [-Wunused-parameter]
   static inline uint16_t inw(uint16_t port)
                              ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'uint32_t hw::inl(uint16_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:50:39: warning: unused parameter 'port' [-Wunused-parameter]
   static inline uint32_t inl(uint16_t port)
                              ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'void hw::outb(uint16_t, uint8_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:62:36: warning: unused parameter 'port' [-Wunused-parameter]
   static inline void outb(uint16_t port, uint8_t data)
                           ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:62:50: warning: unused parameter 'data' [-Wunused-parameter]
   static inline void outb(uint16_t port, uint8_t data)
                                          ~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'void hw::outw(uint16_t, uint16_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:71:36: warning: unused parameter 'port' [-Wunused-parameter]
   static inline void outw(uint16_t port, uint16_t data)
                           ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:71:51: warning: unused parameter 'data' [-Wunused-parameter]
   static inline void outw(uint16_t port, uint16_t data)
                                          ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp: In function 'void hw::outl(uint16_t, uint32_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:80:36: warning: unused parameter 'port' [-Wunused-parameter]
   static inline void outl(uint16_t port, uint32_t data)
                           ~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hw/ioport.hpp:80:51: warning: unused parameter 'data' [-Wunused-parameter]
   static inline void outl(uint16_t port, uint32_t data)
                                          ~~~~~~~~~^~~~
[ 17%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/platform.cpp.o
[ 18%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/kernel_start.cpp.o
[ 19%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/os.cpp.o
[ 20%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/gic.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp: In function 'void gic_init_fdt(const char*, uint32_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp:102:12: warning: variable 'len' set but not used [-Wunused-but-set-variable]
   uint64_t len;
            ^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp: In function 'void init_gicd()':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp:211:7: warning: unused variable 'config_regs' [-Wunused-variable]
   int config_regs=(irq_lines+GIC_V3_GICD_CFGR_PER_REG-1)/GIC_V3_GICD_CFGR_PER_REG;
       ^~~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp: In function 'void __arch_install_irq(uint8_t, uintptr_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/gic.cpp:311:48: warning: unused parameter 'handler' [-Wunused-parameter]
 void __arch_install_irq(uint8_t irq, uintptr_t handler)
                                      ~~~~~~~~~~^~~~~~~
[ 21%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/exception_handling.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp: In function 'void exception_handler_irq_el(stack_frame*, uint64_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:26:51: warning: unused parameter 'ctx' [-Wunused-parameter]
 void exception_handler_irq_el(struct stack_frame *ctx,uint64_t esr)
                               ~~~~~~~~~~~~~~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:26:64: warning: unused parameter 'esr' [-Wunused-parameter]
 void exception_handler_irq_el(struct stack_frame *ctx,uint64_t esr)
                                                       ~~~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp: In function 'void exception_handler_syn_el(stack_frame*, uint64_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:125:11: warning: format '%x' expects argument of type 'unsigned int', but argument 2 has type 'uint64_t' {aka 'long unsigned int'} [-Wformat=]
   kprintf("SYN EXCEPTION %08x\r\n",esr);
           ^~~~~~~~~~~~~~~~~~~~~~~~ ~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp: In function 'void exception_handler_fiq_el(stack_frame*, uint64_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:143:51: warning: unused parameter 'ctx' [-Wunused-parameter]
 void exception_handler_fiq_el(struct stack_frame *ctx,uint64_t esr)
                               ~~~~~~~~~~~~~~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:143:64: warning: unused parameter 'esr' [-Wunused-parameter]
 void exception_handler_fiq_el(struct stack_frame *ctx,uint64_t esr)
                                                       ~~~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp: In function 'void exception_handler_serror_el(stack_frame*, uint64_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:149:54: warning: unused parameter 'ctx' [-Wunused-parameter]
 void exception_handler_serror_el(struct stack_frame *ctx,uint64_t esr)
                                  ~~~~~~~~~~~~~~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/platform/aarch64_vm/exception_handling.cpp:149:67: warning: unused parameter 'esr' [-Wunused-parameter]
 void exception_handler_serror_el(struct stack_frame *ctx,uint64_t esr)
                                                          ~~~~~~~~~^~~
[ 22%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/init_libc.cpp.o
[ 23%] Building CXX object src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/sanity_checks.cpp.o
[ 24%] Linking CXX static library ../../../platform/libaarch64_default.a
[ 24%] Built target aarch64_default
Scanning dependencies of target musl_syscalls
[ 25%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/futex.cpp.o
[ 26%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/close.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/close.cpp:2:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 27%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fcntl.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/fcntl.cpp:2:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 28%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/lseek.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/lseek.cpp:6:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 29%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/ioctl.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/ioctl.cpp:4:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 30%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/writev.cpp.o
[ 31%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/write.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/write.cpp:3:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 32%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/brk.cpp.o
[ 33%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/madvise.cpp.o
[ 34%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mmap.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/kernel/memory.hpp:25,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/typename.hpp:20,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hal/detail/machine.hpp:24,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/hal/machine.hpp:95,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/include/kernel.hpp:20,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/common.hpp:7,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mmap.cpp:1:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_buddy.hpp: In instantiation of 'os::mem::buddy::Alloc<Track_allocs>::Track_res os::mem::buddy::Alloc<Track_allocs>::alloc_tracker(os::mem::buddy::Alloc<Track_allocs>::Track) const [with bool Track_allocs = false]':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_buddy.hpp:326:7:   required from 'void* os::mem::buddy::Alloc<Track_allocs>::allocate(os::mem::buddy::Size_t) [with bool Track_allocs = false; os::mem::buddy::Size_t = long unsigned int]'
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mmap.cpp:31:30:   required from here
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_buddy.hpp:292:35: warning: parameter 'action' set but not used [-Wunused-but-set-parameter]
     Track_res alloc_tracker(Track action = Track::get) const noexcept {
                             ~~~~~~^~~~~~~~~~~~~~~~~~~
[ 35%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mremap.cpp.o
[ 36%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/munmap.cpp.o
[ 37%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/sched_getaffinity.cpp.o
[ 38%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/sysinfo.cpp.o
[ 39%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/prlimit64.cpp.o
[ 40%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/getrlimit.cpp.o
[ 41%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/sched_yield.cpp.o
[ 42%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/set_robust_list.cpp.o
[ 42%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/nanosleep.cpp.o
[ 43%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/open.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/open.cpp:4:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 44%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/creat.cpp.o
[ 45%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/clock_gettime.cpp.o
[ 46%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/gettimeofday.cpp.o
[ 47%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/poll.cpp.o
[ 48%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/exit.cpp.o
[ 49%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/set_tid_address.cpp.o
[ 50%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/pipe.cpp.o
[ 51%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/read.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/read.cpp:2:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 52%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/readv.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/readv.cpp:3:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 53%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/getpid.cpp.o
[ 54%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/getuid.cpp.o
[ 55%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mknod.cpp.o
[ 56%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/sync.cpp.o
[ 57%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/msync.cpp.o
[ 58%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mincore.cpp.o
[ 59%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/syscall_n.cpp.o
[ 60%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/sigmask.cpp.o
[ 61%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/gettid.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/gettid.cpp:5:2: warning: #warning "gettid not implemented for threaded IncludeOS" [-Wcpp]
 #warning "gettid not implemented for threaded IncludeOS"
  ^~~~~~~
[ 62%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/socketcall.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:5:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:38,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/tcp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:6:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp: At global scope:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/ip6/mld.hpp:155:36: warning: 'maybe_unused' attribute ignored [-Wattributes]
       [[maybe_unused]]Mld         &mld_;
                                    ^~~~
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/connection.hpp:34,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/tcp/tcp.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet.hpp:40,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/net/inet:6,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/sockfd.hpp:23,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/tcp_fd.hpp:22,
                 from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:6:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp: In member function 'virtual bool os::mem::Default_pmr::do_is_equal(const std::experimental::fundamentals_v1::pmr::memory_resource&) const':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/util/alloc_pmr.hpp:118:23: warning: unused variable 'underlying' [-Wunused-variable]
       if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
                       ^~~~~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp: In function 'long int socketcall_getsockopt(int, int, int, void*, socklen_t*)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:111:32: warning: unused parameter 'sockfd' [-Wunused-parameter]
 long socketcall_getsockopt(int sockfd,
                            ~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:112:9: warning: unused parameter 'level' [-Wunused-parameter]
     int level, int optname, void *optval, socklen_t *optlen)
     ~~~~^~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:112:20: warning: unused parameter 'optname' [-Wunused-parameter]
     int level, int optname, void *optval, socklen_t *optlen)
                ~~~~^~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:112:35: warning: unused parameter 'optval' [-Wunused-parameter]
     int level, int optname, void *optval, socklen_t *optlen)
                             ~~~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:112:54: warning: unused parameter 'optlen' [-Wunused-parameter]
     int level, int optname, void *optval, socklen_t *optlen)
                                           ~~~~~~~~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp: In function 'long int socketcall_setsockopt(int, int, int, const void*, socklen_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:116:32: warning: unused parameter 'sockfd' [-Wunused-parameter]
 long socketcall_setsockopt(int sockfd,
                            ~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:117:9: warning: unused parameter 'level' [-Wunused-parameter]
     int level, int optname, const void *optval, socklen_t optlen)
     ~~~~^~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:117:20: warning: unused parameter 'optname' [-Wunused-parameter]
     int level, int optname, const void *optval, socklen_t optlen)
                ~~~~^~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:117:41: warning: unused parameter 'optval' [-Wunused-parameter]
     int level, int optname, const void *optval, socklen_t optlen)
                             ~~~~~~~~~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:117:59: warning: unused parameter 'optlen' [-Wunused-parameter]
     int level, int optname, const void *optval, socklen_t optlen)
                                                 ~~~~~~~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp: In function 'long int socketcall_getsockname(int, sockaddr*, socklen_t*)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:121:33: warning: unused parameter 'sockfd' [-Wunused-parameter]
 long socketcall_getsockname(int sockfd,
                             ~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:122:22: warning: unused parameter 'addr' [-Wunused-parameter]
     struct sockaddr *addr, socklen_t *addrlen)
     ~~~~~~~~~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:122:39: warning: unused parameter 'addrlen' [-Wunused-parameter]
     struct sockaddr *addr, socklen_t *addrlen)
                            ~~~~~~~~~~~^~~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp: In function 'long int socketcall_getpeername(int, sockaddr*, socklen_t*)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:127:33: warning: unused parameter 'sockfd' [-Wunused-parameter]
 long socketcall_getpeername(int sockfd,
                             ~~~~^~~~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:128:22: warning: unused parameter 'addr' [-Wunused-parameter]
     struct sockaddr *addr, socklen_t *addrlen)
     ~~~~~~~~~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/socketcall.cpp:128:39: warning: unused parameter 'addrlen' [-Wunused-parameter]
     struct sockaddr *addr, socklen_t *addrlen)
                            ~~~~~~~~~~~^~~~~~~
[ 62%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/rt_sigaction.cpp.o
[ 63%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/stat.cpp.o
[ 64%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fstat.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/fstat.cpp:4:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 65%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fstatat.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/fstatat.cpp:4:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 66%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/access.cpp.o
[ 67%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/chmod.cpp.o
[ 68%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/chown.cpp.o
[ 69%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/cwd.cpp.o
[ 70%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/dup3.cpp.o
[ 71%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/execve.cpp.o
[ 72%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fchmod.cpp.o
[ 73%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fchmodat.cpp.o
[ 74%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fchown.cpp.o
[ 75%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/fsync.cpp.o
[ 76%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/ftruncate.cpp.o
[ 77%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/getdents.cpp.o
In file included from /home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/getdents.cpp:3:
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp: In member function 'void FD_map::internal_close(FD_map::id_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/../api/posix/fd_map.hpp:81:10: warning: unused variable 'erased' [-Wunused-variable]
     auto erased = map_.erase(id);
          ^~~~~~
[ 78%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/geteuid.cpp.o
[ 79%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/getgid.cpp.o
[ 80%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/kill.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/kill.cpp:10:5: warning: #warning "tkill not implemented for threaded IncludeOS" [-Wcpp]
 #   warning "tkill not implemented for threaded IncludeOS"
     ^~~~~~~
[ 81%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mkdir.cpp.o
[ 82%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mkdirat.cpp.o
[ 82%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mknodat.cpp.o
[ 83%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mlock.cpp.o
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp: In function 'long int sys_mlock(const void*, size_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp:4:35: warning: unused parameter 'addr' [-Wunused-parameter]
 static long sys_mlock(const void* addr, size_t len)
                       ~~~~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp:4:48: warning: unused parameter 'len' [-Wunused-parameter]
 static long sys_mlock(const void* addr, size_t len)
                                         ~~~~~~~^~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp: In function 'long int sys_munlock(const void*, size_t)':
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp:8:37: warning: unused parameter 'addr' [-Wunused-parameter]
 static long sys_munlock(const void* addr, size_t len)
                         ~~~~~~~~~~~~^~~~
/home/libreliu/OS/IncludeOS-dev-new/modified_src/src/musl/mlock.cpp:8:50: warning: unused parameter 'len' [-Wunused-parameter]
 static long sys_munlock(const void* addr, size_t len)
                                           ~~~~~~~^~~
[ 84%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/mprotect.cpp.o
[ 85%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/openat.cpp.o
[ 86%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/readlink.cpp.o
[ 87%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/rename.cpp.o
[ 88%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/rmdir.cpp.o
[ 89%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/select.cpp.o
[ 90%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/setgid.cpp.o
[ 91%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/setpgid.cpp.o
[ 92%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/setrlimit.cpp.o
[ 93%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/setsid.cpp.o
[ 94%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/setuid.cpp.o
[ 95%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/wait4.cpp.o
[ 96%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/umask.cpp.o
[ 97%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/uname.cpp.o
[ 98%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/utimensat.cpp.o
[ 99%] Building CXX object src/musl/CMakeFiles/musl_syscalls.dir/unlink.cpp.o
[100%] Linking CXX static library ../../lib/libmusl_syscalls.a
[100%] Built target musl_syscalls
[libreliu@thinkpad-ssd mod_src_build]$ 
```

注意这个路径 `src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/gic.cpp.o` 是由 CMake INSTALL 命令提供的（`INSTALL (TARGET ... DESTINATION ...)`）


> 一个简单的 service（比如 `std::cout << "oops"）怎么实现？要哪些接口实现 `cout`？
> - 可以先用原来的 x86 版本实验一下
原来的：
```
cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

# Human-readable name of your service
set(SERVICE_NAME "IncludeOS seed")

# Name of your service binary
set(BINARY       "seed")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp # ...add more here
  )

# To add your own include paths:
# set(LOCAL_INCLUDES ".")

# DRIVERS / PLUGINS:
set(DRIVERS
  # virtionet   # Virtio networking driver
  # virtioblk   # Virtio block device driver
  boot_logger   # Enable lots of logging from boot stage

  # Use "boot --drivers ." to see other drivers
  )

set(PLUGINS
  # syslogd    # Syslog over UDP

  # Use "boot --plugins ." to see other plugins
  )

# STATIC LIBRARIES:
set(LIBRARIES
  # path to full library
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

# Create in-memory filesystem from folder
#diskbuilder(my_folder)
```

```
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

add_executable 在 `/IncludeOS_Install/includeos/post.service.cmake` 里面。
```
# executable
set(SERVICE_STUB "${INSTALL_LOC}/src/service_name.cpp")

add_executable(service ${SOURCES} ${SERVICE_STUB})
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})
```

所以，应该看一下 `default_stdout` 的路径，这样我们才可以考虑打印的事情。

**注意，cmake 可以有 `-LH`（打印variables和他们的help），以及`--trace`（跟踪每行执行的指令），会比较有用.jpg**

*UPDATE*: 打印的事情参考 [`stdout.md`](stdout.md)

### make it editable
因为新版本 hello_world 引用了 includeos 来进行构建，所以我们得把我们的更改后的包创建。但是每次都创建过于麻烦，比较好的方法是参考 IncludeOS README.md 那里把包设置成 editable 模式，直接调用这里。

首先改 `etc/layout.txt` ，我这里是 `{% set build_dir='../mod_src_build' %}`。

然后如下：
```
[libreliu@thinkpad-ssd modified_src]$ conan editable add . includeos/$(conan inspect -a version . | cut -d " " -f 2)@includeos/latest --layout=etc/layout.txt
Using layout file: /home/libreliu/OS/IncludeOS-dev-new/modified_src/etc/layout.txt
Reference 'includeos/0.0.0@includeos/latest' in editable mode
[libreliu@thinkpad-ssd modified_src]$ 
```

> 0.0.0 是因为从 git 出来我先用了 conda source，从而在 `modified_src` 里面没有 `.git` 了吧..版本号是和 git 相关的。
```
[libreliu@thinkpad-ssd mod_src_build]$ conan editable remove includeos/0.0.0@includeos/latest
Removed editable mode for reference 'includeos/0.0.0@includeos/latest'
```
```
[libreliu@thinkpad-ssd mod_src_build]$ conan create ../modified_src/ includeos/latest -pr gcc-8.2.0-linux-aarch64
Exporting package recipe
ERROR: Not a valid 'git' repository
```
生成不了版本号，先把 `.git` 拷进去救火233
```
[libreliu@thinkpad-ssd mod_src_build]$ conan create ../modified_src/ includeos/latest -pr gcc-8.2.0-linux-aarch64
Exporting package recipe
includeos/0.14.2-1208@includeos/latest: Repo origin deduced by 'auto': https://github.com/IncludeOS/IncludeOS.git
includeos/0.14.2-1208@includeos/latest: WARN: Repo status is not pristine: there might be modified files
includeos/0.14.2-1208@includeos/latest: Revision deduced by 'auto': 7f4049a5a45f91a99cc059520c768acf33807085
includeos/0.14.2-1208@includeos/latest: A new conanfile.py version was exported
includeos/0.14.2-1208@includeos/latest: Folder: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/export
Configuration:
[settings]
arch=armv8
arch_build=x86_64
build_type=Release
compiler=gcc
compiler.libcxx=libstdc++11
compiler.version=8
os=Linux
os_build=Linux
[options]
[build_requires]
*: binutils/2.31@includeos/toolchain
[env]
CC=aarch64-linux-gnu-gcc-8
CFLAGS=-mcpu=cortex-a53 -O2 -g
CXX=aarch64-linux-gnu-g++-8
CXXFLAGS=-mcpu=cortex-a53 -O2 -g
includeos/0.14.2-1208@includeos/latest: WARN: Forced build from source
Version ranges solved
    Version range '>=1.0.0' required by 'python_require' resolved to 'conan-tools/1.0.0@includeos/stable'
    Version range '>=5.0' required by 'includeos/0.14.2-1208@includeos/latest' resolved to 'libcxx/7.0.1@includeos/stable'
    Version range '>=1.1.18' required by 'libcxx/7.0.1@includeos/stable' resolved to 'musl/1.1.18@includeos/stable'

Installing package: includeos/0.14.2-1208@includeos/latest
Requirements
    GSL/2.0.0@includeos/stable from 'includeos' - Cache
    includeos/0.14.2-1208@includeos/latest from 'includeos' - Cache
    libcxx/7.0.1@includeos/stable from 'includeos' - Cache
    libcxxabi/7.0.1@includeos/stable from 'includeos' - Cache
    libfdt/1.4.7@includeos/stable from 'includeos' - Cache
    libgcc/1.0@includeos/stable from 'includeos' - Cache
    libunwind/7.0.1@includeos/stable from 'includeos' - Cache
    musl/1.1.18@includeos/stable from 'includeos' - Cache
Python requires
    conan-tools/1.0.0@includeos/stable
Packages
    GSL/2.0.0@includeos/stable:5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9 - Cache
    includeos/0.14.2-1208@includeos/latest:b3018e29d4dd50972873977ff12deeeb646c9d92 - Build
    libcxx/7.0.1@includeos/stable:4680b3386fce626bbd782cd807fa700f3de361fa - Cache
    libcxxabi/7.0.1@includeos/stable:a38d7c31d4564d43a7705539d94b1907c6418a85 - Cache
    libfdt/1.4.7@includeos/stable:c3cd523f63cc051ff1178d8c88ede55d315cf9be - Cache
    libgcc/1.0@includeos/stable:a38d7c31d4564d43a7705539d94b1907c6418a85 - Cache
    libunwind/7.0.1@includeos/stable:c3cd523f63cc051ff1178d8c88ede55d315cf9be - Cache
    musl/1.1.18@includeos/stable:d5bf95d2a20952225177c123a6a3da87fec0744b - Cache
Build requirements
    binutils/2.31@includeos/toolchain from 'includeos' - Cache
Build requirements packages
    binutils/2.31@includeos/toolchain:1907037da8323f14f8235c9a3fabcb665a84c867 - Cache

Cross-build from 'Linux:x86_64' to 'Linux:armv8'
GSL/2.0.0@includeos/stable: Already installed!
binutils/2.31@includeos/toolchain: Already installed!
libcxxabi/7.0.1@includeos/stable: Already installed!
libfdt/1.4.7@includeos/stable: Already installed!
libgcc/1.0@includeos/stable: Already installed!
libunwind/7.0.1@includeos/stable: Already installed!
musl/1.1.18@includeos/stable: Already installed!
libcxx/7.0.1@includeos/stable: Already installed!
includeos/0.14.2-1208@includeos/latest: Applying build-requirement: binutils/2.31@includeos/toolchain
includeos/0.14.2-1208@includeos/latest: WARN: Detected 'scm' auto in conanfile, trying to remove source folder
includeos/0.14.2-1208@includeos/latest: WARN: This can take a while for big packages
includeos/0.14.2-1208@includeos/latest: Configuring sources in /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source
includeos/0.14.2-1208@includeos/latest: Getting sources from folder: /home/libreliu/OS/IncludeOS-dev-new/modified_src
includeos/0.14.2-1208@includeos/latest: Building your package in /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/build/b3018e29d4dd50972873977ff12deeeb646c9d92
includeos/0.14.2-1208@includeos/latest: Generator cmake created conanbuildinfo.cmake
includeos/0.14.2-1208@includeos/latest: Generator virtualenv created activate.sh
includeos/0.14.2-1208@includeos/latest: Generator virtualenv created deactivate.sh
includeos/0.14.2-1208@includeos/latest: Calling build()
CMake Error at /usr/share/cmake-3.14/Modules/CMakeDetermineCCompiler.cmake:49 (message):
  Could not find compiler set in environment variable CC:

  aarch64-linux-gnu-gcc-8.
Call Stack (most recent call first):
  CMakeLists.txt:6 (project)


CMake Error: CMAKE_C_COMPILER not set, after EnableLanguage
CMake Error: CMAKE_CXX_COMPILER not set, after EnableLanguage
-- Configuring incomplete, errors occurred!
See also "/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/build/b3018e29d4dd50972873977ff12deeeb646c9d92/CMakeFiles/CMakeOutput.log".
includeos/0.14.2-1208@includeos/latest: 
includeos/0.14.2-1208@includeos/latest: ERROR: Package 'b3018e29d4dd50972873977ff12deeeb646c9d92' build failed
includeos/0.14.2-1208@includeos/latest: WARN: Build folder /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/build/b3018e29d4dd50972873977ff12deeeb646c9d92
ERROR: includeos/0.14.2-1208@includeos/latest: Error in build() method, line 84
	cmake=self._configure_cmake()
while calling '_configure_cmake', line 80
	cmake.configure(source_folder=self.source_folder)
	ConanException: Error 256 while executing cd '/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/build/b3018e29d4dd50972873977ff12deeeb646c9d92' && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release" -DCONAN_EXPORTED="1" -DCONAN_IN_LOCAL_CACHE="ON" -DCONAN_COMPILER="gcc" -DCONAN_COMPILER_VERSION="8" -DCMAKE_INSTALL_PREFIX="/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92" -DCMAKE_INSTALL_BINDIR="bin" -DCMAKE_INSTALL_SBINDIR="bin" -DCMAKE_INSTALL_LIBEXECDIR="bin" -DCMAKE_INSTALL_LIBDIR="lib" -DCMAKE_INSTALL_INCLUDEDIR="include" -DCMAKE_INSTALL_OLDINCLUDEDIR="include" -DCMAKE_INSTALL_DATAROOTDIR="share" -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY="ON" -DVERSION="0.14.2-1208" -DPLATFORM="default" -DSMP="False" -Wno-dev '/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source'
```
> 这里可以发现，`activate.sh` 是 virtualenv 包提供的..

应该是没加载`activate.sh`，试试

还是不行，改一下 profile 里面的 env，去掉 `-8`

这回可以了：
```
...(skip some lines)... # 这里包括编译 (CMake) 和 Install（CMake Install 似乎会被 conan 重载，安装到这个pkg的地方里）
/package/b3018e29d4dd50972873977ff12deeeb646c9d92/aarch64/lib/libarch.a
-- Installing: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/aarch64/linker.ld
-- Installing: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/platform/libaarch64_default.a
-- Installing: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/lib/libmusl_syscalls.a
includeos/0.14.2-1208@includeos/latest package(): Packaged 4 '.a' files: libarch.a, libmusl_syscalls.a, libos.a, libaarch64_default.a
includeos/0.14.2-1208@includeos/latest package(): Packaged 1 '.ld' file: linker.ld
includeos/0.14.2-1208@includeos/latest package(): Packaged 3 '.cmake' files: includeos.cmake, linux.service.cmake, os.cmake
includeos/0.14.2-1208@includeos/latest package(): Packaged 246 '.hpp' files
includeos/0.14.2-1208@includeos/latest package(): Packaged 40 files
includeos/0.14.2-1208@includeos/latest package(): Packaged 4 '.h' files: multiboot.h, auxvec.h, stddef.h, memstream.h
includeos/0.14.2-1208@includeos/latest package(): Packaged 2 '.inc' files: connection.inc, elf_binary.inc
includeos/0.14.2-1208@includeos/latest package(): Packaged 1 '.cpp' file: service_name.cpp
includeos/0.14.2-1208@includeos/latest package(): Packaged 2 '.asm' files: empty.asm, memdisk.asm
includeos/0.14.2-1208@includeos/latest package(): Packaged 1 '.py' file: memdisk.py
includeos/0.14.2-1208@includeos/latest: Package 'b3018e29d4dd50972873977ff12deeeb646c9d92' created
includeos/0.14.2-1208@includeos/latest: Created package revision 8e906c46f3b5abdd520cda5842434c0a
(conanenv) [libreliu@thinkpad-ssd mod_src_build]$ 
```