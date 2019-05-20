# 构建 IncludeOS
这篇文章是 [conan.md](conan.md) 的整理，因为原来的写的太乱了。

## IncludeOS 的传统架构 (master 分支，不用 Conan)
1. 构建 IncludeOS 的各种库（pre install）
2. 构建 IncludeOS 的组件
3. 构建完成的东西（各种 `.a`）放到 "Installed" 文件夹，设置环境变量
4. （应用开发者）构建自己的 Service，这个过程中会调用 `IncludeOS_Install（你的安装文件夹）/includeos/` 里面的 `post.service.cmake` 和 `pre.service.cmake`；这些会设置一些构建的参数，linker script 什么的
5. 构建完成，调用 vmbuilder 啥的生成可引导 bin（可以参见 dev 分支 `README.md`，有些说明）

## IncludeOS 的 `dev` 分支
- 把 vmrunner/vmbuilder 都弄成了单独的 Conan package
- 把构建好的 IncludeOS 也弄成了 package
  - 这样应用开发者自己的 Service 可以直接调用 Conan 把 IncludeOS 安装好

那么我们要做的就是构建 IncludeOS 的这个 package。

### 步骤记录
因为编译很烦杂，所以做些记录以待（考完数理方程）全组研究考证。

#### 紫檀同学的尝试

1. `git clone https://github.com/IncludeOS/IncludeOS.git -b dev`
2. `cd IncludeOS && make ../deployed_src`
3. `conan source . --source-folder=../deployed_src`
  - 其实这步是冗余的，因为 IncludeOS 这个包没有配置「external sources」，所以调用 `source` 并不会增加任何新东西，而在复制到另一个 Source Folder 的过程中会损失掉 `.git` 目录；IncludeOS 的 `conanfile.py` 中确定版本号的方式是采用 git 版本串的一部分作自增，自动生成一个合适的版本号，而没有这个版本号信息会在创建 IncludeOS 包的步骤中（即 `conan create`）出问题。
4. `cd .. && cp -r deployed_src/ modified_src/ && cd modified_src`
  - 这里我为了作更改更方便..
5. 打开 `conanfile.py`，编辑掉 `if not ... nano` 下面的 `require` 项
  - 这里是因为编译 aarch64 的时候，那些奇怪的依赖项经常编译不过去（比如 botan）；我们现在也没达到真正需要那些依赖的程度，故去掉
6. `conan install . --install-folder=../mod_src_build -pr gcc-8.2.0-linux-aarch64 -e CC=aarch64-linux-gnu-gcc -e CXX=aarch64-linux-gnu-g++`
  - 默认的 profile 里面
7. 现在在`../mod_src_build`目录下有了 `activate.sh`
  - `activate.sh` 用来设置 `CXX` `C` `CXXFLAGS` `CFLAGS` `PATH` 等
  - 还生成了 `conaninfo.txt` 和 `conanbuildinfo.cmake` 等物，后面那个是直接被 CMake include 的文件
8. `conan build . -bf ../mod_src_build` （在 `modified_src` 下运行）
  - `-bf` 是制定 build folder, 也就是 build 的目标文件夹
  - **这时，会出现找不到 `rapidjson` 的错误**，所以我们尝试注释掉 `src/CMakeLists.txt` 里面的 `Libraries`
    - 我一气全注释了emmm
  - 关系如下：
    - Conan 负责外部库的 includePath 等，以及静态库构建
    - IncludeOS 自己的构建则用 CMake 本身解决！
    - 参见 `src/CMakeLists.txt`，通过先定义「LIBARIES」变量，然后用`add_subdirectory()`调用，进而调用里面的子的 `CMakeLists.txt` 这样子。
    - 所以现在 Conan 已经基本不用动了。~~每次改的话，用 conan export 弄到 local cache 然后 Conan build 就行吧..~~ 似乎要用 `conan create`把改好的包看看能不能打包成功（不过 *conan editable* 似乎也可以，有人研究一下吗？）
9. 开发的过程
  - 如果再运行一遍 CMake 的话，CMake 会重新解析那些依赖的 CMakeLists.txt，所以如果源码有改动（eg 增加了新的 `.c` **而要在对应的 `src` 的目录下改动 `CMakeLists.txt`**），就要重新`cmake ../mod_src_build/`。
  - 然后再 `cd ../mod_src_build && make`，和正常 CMake 项目一致。
  - 全注释了的 `make` 输出（节选）：
```
[100%] Linking CXX static library ../../lib/libmusl_syscalls.a
[100%] Built target musl_syscalls
[libreliu@thinkpad-ssd mod_src_build]$ 
```
  - 注意这个路径 `src/platform/aarch64_vm/CMakeFiles/aarch64_default.dir/gic.cpp.o`什么的是由 CMake INSTALL 命令提供的（`INSTALL (TARGET ... DESTINATION ...)`）

10. 打包（把包构建一遍，放到 local cache）：`conan create ../modified_src/ includeos/latest -pr gcc-8.2.0-linux-aarch64`
  - 打包需要 `.git` 文件夹，所以我就提前又把 `dev` 分支的 `.git` 拷回`modified_src/`去救火了
  - *默认的 aarch64 Profile* 里面的编译器的名称和我的不一样（它的：`aarch64-linux-gnu-gcc-8`，我的：`aarch64-linux-gnu-gcc`）；我把我的`gcc-8.2.0-linux-aarch64` 进行了一番修改
  - 打包之前最好先加载 `source activate.sh` 脚本（此条存疑）
  - 打包是因为如 `hello_world` 这样的 Service 是通过包依赖的方式来获得 IncludeOS 和其各种依赖库的链接和脚本的，所以不得不打包；我还不知道不用 Service 而直接开启 IncludeOS 是否可行（多半不行）
  - 这里可以发现，`activate.sh` 是 virtualenv 包提供的

11. 总之经历刚才的折腾，我顺利打包了：打包后的版本`version: 0.14.2-1208`，并且放到了 local cache 里面。
  - 之前我让包进 `editable` 的时候，因为没有 `.git`，包名被定为 `includeos/0.0.0@includeos/latest`（233），不过这个时候改一下 hello_world 里面 `conanfile.txt` 似乎也可？但是我没能成功使用 editable 的本地包（就不是 create 后的包，那个在 local cache，而是在我的`/home/libreliu/OS/IncludeOS-dev-new/modified_src`的“包”，不知为何。（TODO）

12. `git clone https://github.com/Includeos/hello_world.git`
  - 它的官网里面的仓库有些不要 Conan （给 master 用的），有些要（给现在还正在 dev 的这个分支准备的）。
13. `cd hello_world && mkdir my_build && cd my)build`
  - 这个是方便隔离 CMake 产生的众多的临时文件（是写 CMake 项目时候编译的常规操作）
  - 这时候尝试 `cmake ..` CMake 会提示没找到 `conda_basic_setup` 这个宏（函数），这个函数在 `conanbuildinfo.cmake` 里面，但是因为还没有 `conan install` 故而没有。
14. `conan install .. -pr gcc-8.2.0-linux-aarch64 && source activate.sh`
> `source <script_file>` 和 `. <script_file>` 意义相同，都表示用 shell 执行一下这个脚本；但是和 `sh <script_file>` 不同的是，后者 fork 子进程执行，而前者直接在父进程里面执行，有点像 C 语言预处理器的 `#include`。
15. `cmake ..`
  - 不过我这样尝试失败了，CMake 并不能乖乖 include 上 `conanbuildinfo.cmake`。（应该是因为`${CMAKE_CURRENT_BINARY_DIR}` 会展开到 CMakeLists.txt 所在的地方？）
  - 我尝试把 `my_build` 里面的所有东西都拷贝到上层目录（`hello_world/`，也就是放 CMakeLists.txt 的目录），再 `cmake .`（在`hello_world/`下运行）才成功。
  - 然后应该会提示找不到 `libdefault_stdout.a` 啥的，因为有个 `os_add_stdout()` 宏在 `hello_world/CMakeLists.txt` 里面。
  - 我还没看 `OS::add_stdout` 这块的代码，所以我直接把`CMakeLists.txt`里`os_add_stdout()`这行也注释了emmm
16. `make`
然后就会出现这个问题：
```
(conanenv) [libreliu@thinkpad-ssd hello_world]$ make
[ 25%] Linking CXX executable bin/hello.elf.bin
/usr/bin/ld: 无法辨认的仿真模式: aarch64elf
支持的仿真： elf_x86_64 elf32_x86_64 elf_i386 elf_iamcu elf_l1om elf_k1om i386pep i386pe
make[2]: *** [CMakeFiles/hello.elf.bin.dir/build.make:116：bin/hello.elf.bin] 错误 1
make[1]: *** [CMakeFiles/Makefile2:73：CMakeFiles/hello.elf.bin.dir/all] 错误 2
make: *** [Makefile:84：all] 错误 2
```
为啥它用这个 ld 我也不是很清楚emmm，是不是 vmbuilder 一系列工具的锅（他们会不会无视env？瞎猜）
> 这里是不是只能 `cmake --trace` 或者 `make` 的类似 trace 功能？
- 我尝试`cmake . -D CMAKE_C_LINK_EXECUTABLE=aarch64-linux-gnu-ld -DCMAKE_CXX_LINK_EXECUTABLE=aarch64-linux-gnu-ld` 再 `make`，没啥效果

------
更新：
17. 把 `src/drivers/CMakeLists.txt` 改了改，现在可以编译 `libdefault_stdout.a` 了。重新打包。
18. 调戏 CMake：CMake 分两种变量，所以清除 Cache （`rm CMakeCache.txt`「必须要用，否则`-D`的没有效果」） 再 
`cmake -DCMAKE_LINKER=aarch64-linux-gnu-ld` works（如果不行，有些 CMake 版本需要这个：` -DCMAKE_CXX_LINK_EXECUTABLE="<CMAKE_LINKER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"`）：
```
(conanenv) [libreliu@thinkpad-ssd hello_world]$ cmake -DCMAKE_LINKER=aarch64-linux-gnu-ld                                                                      
CMake Warning:
  No source or binary directory provided.  Both will be assumed to be the
  same as the current working directory, but note that this warning will
  become a fatal error in future CMake releases.


-- Conan: Adjusting output directories
-- Conan: Using cmake global configuration
-- Conan: Adjusting default RPATHs Conan policies
-- Conan: Adjusting language standard
-- Current conanbuildinfo.cmake directory: /home/libreliu/OS/IncludeOS-dev-new/hello_world
-- Conan: Compiler GCC>=5, checking major version 8
-- Conan: Checking correct version: 8
-- Library os found /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/lib/libos.a
-- Library arch not found in package, might be system one
-- Library musl_syscalls found /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/lib/libmusl_syscalls.a
-- Library aarch64_pc not found in package, might be system one
-- Library c++ found /home/libreliu/.conan/data/libcxx/7.0.1/includeos/stable/package/4680b3386fce626bbd782cd807fa700f3de361fa/lib/libc++.a
-- Library c++experimental found /home/libreliu/.conan/data/libcxx/7.0.1/includeos/stable/package/4680b3386fce626bbd782cd807fa700f3de361fa/lib/libc++experimental.a
-- Library compiler found /home/libreliu/.conan/data/libgcc/1.0/includeos/stable/package/a38d7c31d4564d43a7705539d94b1907c6418a85/lib/libcompiler.a
-- Library fdt found /home/libreliu/.conan/data/libfdt/1.4.7/includeos/stable/package/c3cd523f63cc051ff1178d8c88ede55d315cf9be/lib/libfdt.a
-- Library c found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a
-- Library crypt found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libcrypt.a
-- Library m found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libm.a
-- Library rt found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/librt.a
-- Library dl found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libdl.a
-- Library pthread found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libpthread.a
-- Library resolv found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libresolv.a
-- Library util found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libutil.a
-- Library xnet found /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libxnet.a
-- Library unwind found /home/libreliu/.conan/data/libunwind/7.0.1/includeos/stable/package/c3cd523f63cc051ff1178d8c88ede55d315cf9be/lib/libunwind.a
-- Library c++abi found /home/libreliu/.conan/data/libcxxabi/7.0.1/includeos/stable/package/a38d7c31d4564d43a7705539d94b1907c6418a85/lib/libc++abi.a
-- Configuring done
-- Generating done
-- Build files have been written to: /home/libreliu/OS/IncludeOS-dev-new/hello_world
(conanenv) [libreliu@thinkpad-ssd hello_world]$ make
[ 25%] Linking CXX executable bin/hello.elf.bin
/usr/bin/aarch64-linux-gnu-ld: cannot open linker script file /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/b3018e29d4dd50972873977ff12deeeb646c9d92/linker.ld: No such file or directory
make[2]: *** [CMakeFiles/hello.elf.bin.dir/build.make:117: bin/hello.elf.bin] Error 1
make[1]: *** [CMakeFiles/Makefile2:73: CMakeFiles/hello.elf.bin.dir/all] Error 2
make: *** [Makefile:84: all] Error 2
```
把 src/arch/aarch64 那个拷进去，就会出
```
(conanenv) [libreliu@thinkpad-ssd hello_world]$ make
[ 25%] Linking CXX executable bin/hello.elf.bin
/usr/bin/aarch64-linux-gnu-ld: cannot find -larch
/usr/bin/aarch64-linux-gnu-ld: cannot find -laarch64_pc
make[2]: *** [CMakeFiles/hello.elf.bin.dir/build.make:117: bin/hello.elf.bin] Error 1
make[1]: *** [CMakeFiles/Makefile2:73: CMakeFiles/hello.elf.bin.dir/all] Error 2
make: *** [Makefile:84: all] Error 2
```

这个问题是因为 Linker 的相关设定是在 `cmake/os.cmake`：
```cmake
set(LINK_SCRIPT ${CONAN_RES_DIRS_INCLUDEOS}/linker.ld)
#includeos package can provide this!
include_directories(
  ${CONAN_RES_DIRS_INCLUDEOS}/include/os
)

# TODO: find a more proper way to get the linker.ld script ?
if("${ARCH}" STREQUAL "aarch64")
  set(LDFLAGS "-nostdlib -m${ELF}elf --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT} ${PROD_USE} ${PRE_BSS_SIZE}")
else()
  set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT} ${PROD_USE} ${PRE_BSS_SIZE}")
endif()
```
而安装 Linker Script 的事情是在 `src/arch/i686/CMakeLists.txt` 处理的：
```cmake
set_target_properties(arch PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS arch DESTINATION lib)
install(FILES linker.ld DESTINATION .)
configure_file(linker.ld ${CMAKE_BINARY_DIR})
```
而 AArch64 的实现稍有不同：`src/arch/aarch64/CMakeLists.txt`：
```cmake
set_target_properties(arch PROPERTIES LINKER_LANGUAGE CXX)
configure_file(linker.ld ${CMAKE_BINARY_DIR})
install(TARGETS arch DESTINATION ${ARCH}/lib)
install(FILES linker.ld DESTINATION ${ARCH})
```
所以 linker.ld 和 libarch.a 被装在了包的 `aarch64` 目录下面，难怪找不到。
> 改一下 os.cmake 试一下。



-----

```
(conanenv) [libreliu@thinkpad-ssd hello_world]$ make
Scanning dependencies of target hello.elf.bin
[ 25%] Building CXX object CMakeFiles/hello.elf.bin.dir/main.cpp.o
[ 50%] Building CXX object CMakeFiles/hello.elf.bin.dir/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/src/service_name.cpp.o
[ 75%] Linking CXX executable bin/hello.elf.bin
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(vfprintf.lo): in function `pop_arg':
vfprintf.c:(.text.pop_arg+0x11c): undefined reference to `__extenddftf2'
vfprintf.c:(.text.pop_arg+0x11c): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(vfprintf.lo): in function `fmt_fp':
vfprintf.c:(.text.fmt_fp+0xc0): undefined reference to `__addtf3'
vfprintf.c:(.text.fmt_fp+0xc0): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0xe0): undefined reference to `__netf2'
vfprintf.c:(.text.fmt_fp+0xe0): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x118): undefined reference to `__netf2'
vfprintf.c:(.text.fmt_fp+0x118): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x14c): undefined reference to `__fixunstfsi'
vfprintf.c:(.text.fmt_fp+0x14c): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__fixunstfsi'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x154): undefined reference to `__floatunsitf'
vfprintf.c:(.text.fmt_fp+0x154): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x168): undefined reference to `__subtf3'
vfprintf.c:(.text.fmt_fp+0x168): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x178): undefined reference to `__multf3'
vfprintf.c:(.text.fmt_fp+0x178): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__multf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x198): undefined reference to `__netf2'
vfprintf.c:(.text.fmt_fp+0x198): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x47c): undefined reference to `__addtf3'
vfprintf.c:(.text.fmt_fp+0x47c): relocation truncated to fit: R_AARCH64_CALL26 against undefined symbol `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x490): undefined reference to `__netf2'
vfprintf.c:(.text.fmt_fp+0x490): additional relocation overflows omitted from the output
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x7fc): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x828): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x834): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x8fc): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x914): undefined reference to `__fixtfsi'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x924): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x938): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x948): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x980): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0xb4c): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0xc3c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x1614): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x162c): undefined reference to `__fixtfsi'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x163c): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x1650): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x1660): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x1cd8): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: vfprintf.c:(.text.fmt_fp+0x1ce8): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(strtod.lo): in function `strtof_l':
strtod.c:(.text.strtof+0x6c): undefined reference to `__trunctfsf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(strtod.lo): in function `strtod_l':
strtod.c:(.text.strtod+0x6c): undefined reference to `__trunctfdf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(wcstod.lo): in function `wcstof':
wcstod.c:(.text.wcstof+0xc): undefined reference to `__trunctfsf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(wcstod.lo): in function `wcstod':
wcstod.c:(.text.wcstod+0xc): undefined reference to `__trunctfdf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(floatscan.lo): in function `decfloat':
floatscan.c:(.text.decfloat+0x21c): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x228): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x238): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x330): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x4ac): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x500): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x50c): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x514): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x524): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x828): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x830): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x858): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x870): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x878): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x8a4): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x8b8): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x8c0): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x8e8): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x904): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x90c): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x914): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x958): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x978): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x994): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x9a0): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x9e4): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0x9f0): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xa20): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xa28): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xb08): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xb18): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xb28): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xb34): undefined reference to `__trunctfdf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xb6c): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xbc8): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xbd8): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xbe8): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xc08): undefined reference to `__eqtf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xc20): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xc5c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xc90): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xc98): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xce0): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xcec): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xd00): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.decfloat+0xd10): undefined reference to `__divtf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(floatscan.lo): in function `__floatscan':
floatscan.c:(.text.__floatscan+0x738): undefined reference to `__getf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x758): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x764): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x7b4): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x7c0): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x7e4): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x804): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x810): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x81c): undefined reference to `__floatunsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x828): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x830): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x83c): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x844): undefined reference to `__subtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x850): undefined reference to `__eqtf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x88c): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x89c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x8ac): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x90c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x918): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x920): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x92c): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x964): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x970): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x9a4): undefined reference to `__addtf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0x9d8): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0xb04): undefined reference to `__floatsitf'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0xb14): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: floatscan.c:(.text.__floatscan+0xb24): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(fmodl.lo): in function `fmodl':
fmodl.c:(.text.fmodl+0x28): undefined reference to `__eqtf2'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x38): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x40): undefined reference to `__divtf3'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0xb0): undefined reference to `__letf2'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0xdc): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x10c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x188): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x1ac): undefined reference to `__eqtf2'
/usr/bin/aarch64-linux-gnu-ld: fmodl.c:(.text.fmodl+0x268): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(frexpl.lo): in function `frexpl':
frexpl.c:(.text.frexpl+0x70): undefined reference to `__netf2'
/usr/bin/aarch64-linux-gnu-ld: frexpl.c:(.text.frexpl+0xa4): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(scalbnl.lo): in function `scalbnl':
scalbnl.c:(.text.scalbnl+0x28): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: scalbnl.c:(.text.scalbnl+0x4c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: scalbnl.c:(.text.scalbnl+0x7c): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: scalbnl.c:(.text.scalbnl+0xb4): undefined reference to `__multf3'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(scalbnl.lo):scalbnl.c:(.text.scalbnl+0xd8): more undefined references to `__multf3' follow
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(vfscanf.lo): in function `__isoc99_vfscanf':
vfscanf.c:(.text.vfscanf+0x890): undefined reference to `__trunctfsf2'
/usr/bin/aarch64-linux-gnu-ld: vfscanf.c:(.text.vfscanf+0xd60): undefined reference to `__trunctfdf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(vfwprintf.lo): in function `pop_arg':
vfwprintf.c:(.text.pop_arg+0x11c): undefined reference to `__extenddftf2'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libos.a(c_abi.c.o): in function `_move_symbols':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/crt/c_abi.c:37: undefined reference to `_get_elf_section_datasize'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/crt/c_abi.c:41: undefined reference to `_move_elf_syms_location'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(writev.cpp.o): in function `sys_writev':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/writev.cpp:13: undefined reference to `os::print(char const*, unsigned long)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(mmap.cpp.o): in function `kalloc':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/mmap.cpp:30: undefined reference to `kernel::heap_ready()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(mmap.cpp.o): in function `__expect_fail(char const*, char const*, int, char const*)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/common:70: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(open.cpp.o): in function `fs::VFS_entry& fs::VFS::get_entry<char const*>(char const*)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/fs/vfs.hpp:360: undefined reference to `fs::Path::Path(std::initializer_list<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > >)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/fs/vfs.hpp:364: undefined reference to `fs::Path::to_string() const'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(open.cpp.o): in function `fs::Dirent fs::VFS::stat_sync<char const*>(char const*)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/fs/vfs.hpp:392: undefined reference to `fs::Path::Path(std::initializer_list<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > >)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/fs/vfs.hpp:396: undefined reference to `fs::Path::to_string() const'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(open.cpp.o): in function `File_FD& FD_map::open<File_FD, fs::Dirent&>(fs::Dirent&)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/posix/fd_map.hpp:95: undefined reference to `fs::Dirent::Dirent(fs::Dirent const&)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(open.cpp.o): in function `File_FD::File_FD(int, fs::Dirent, unsigned long)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/posix/file_fd.hpp:28: undefined reference to `vtable for File_FD'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/posix/file_fd.hpp:28: undefined reference to `vtable for File_FD'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/posix/file_fd.hpp:28: undefined reference to `fs::Dirent::Dirent(fs::Dirent const&)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o): in function `sys_kill(int, int)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:5: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o): in function `sys_tkill(int, int)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:12: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o): in function `sys_tgkill(int, int, int)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:16: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o): in function `sys_kill(int, int)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:5: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o): in function `sys_tkill(int, int)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:12: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(kill.cpp.o):/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/kill.cpp:16: more undefined references to `os::panic(char const*)' follow
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(platform.cpp.o): in function `__platform_init(unsigned long)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:59: undefined reference to `Events::get(int)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:59: undefined reference to `Events::init_local()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:73: undefined reference to `Timers::init(delegate<void (std::__1::chrono::duration<long long, std::__1::ratio<1l, 1000000000l> >), spec::inplace, 32ul, 16ul> const&, delegate<void (), spec::inplace, 32ul, 16ul> const&)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:99: undefined reference to `Timers::timers_handler()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:99: undefined reference to `Timers::timers_handler()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:114: undefined reference to `RTC::init()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:116: undefined reference to `Timers::ready()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(platform.cpp.o): in function `RNG::init()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/platform.cpp:133: undefined reference to `rng_absorb(void const*, unsigned long)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(kernel_start.cpp.o): in function `kernel_start':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/kernel_start.cpp:237: undefined reference to `os::Machine::create(void*, unsigned long)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/kernel_start.cpp:239: undefined reference to `_init_elf_parser'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/kernel_start.cpp:241: undefined reference to `os::Machine::init()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/kernel_start.cpp:244: undefined reference to `_init_syscalls'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(init_libc.cpp.o): in function `kernel_main':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/init_libc.cpp:39: undefined reference to `kernel::state()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/init_libc.cpp:64: undefined reference to `kernel::post_start()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(init_libc.cpp.o): in function `aarch64::init_libc(unsigned long)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/init_libc.cpp:105: undefined reference to `Service::name()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(init_libc.cpp.o): in function `rng_extract_uint64()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/../api/kernel/rng.hpp:48: undefined reference to `rng_extract(void*, unsigned long)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(sanity_checks.cpp.o): in function `kernel_sanity_checks':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/sanity_checks.cpp:69: undefined reference to `Elf::verify_symbols()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/sanity_checks.cpp:71: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/sanity_checks.cpp:76: undefined reference to `os::panic(char const*)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(os.cpp.o): in function `os::nanos_asleep()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:22: undefined reference to `os::cpu_freq()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(os.cpp.o): in function `kernel::start(unsigned long)':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:31: undefined reference to `Service::binary_name()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:31: undefined reference to `kernel::state()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:35: undefined reference to `os::add_stdout(delegate<void (char const*, unsigned long), spec::inplace, 32ul, 16ul>)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(os.cpp.o): in function `os::event_loop()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:66: undefined reference to `Events::get(int)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:66: undefined reference to `Events::process_events()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:69: undefined reference to `Events::get(int)'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:69: undefined reference to `Events::process_events()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(os.cpp.o): in function `kernel::is_running()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/include/kernel.hpp:54: undefined reference to `kernel::state()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/platform/libaarch64_default.a(os.cpp.o): in function `os::event_loop()':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/platform/aarch64_vm/os.cpp:73: undefined reference to `Service::stop()'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/musl/1.1.18/includeos/stable/package/d5bf95d2a20952225177c123a6a3da87fec0744b/lib/libc.a(_Exit.lo): in function `_Exit':
_Exit.c:(.text._Exit+0xc): undefined reference to `syscall_SYS_exit_group'
/usr/bin/aarch64-linux-gnu-ld: /home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/package/43d207b59cc47a081f1cd51c720df4739cb1654a/lib/libmusl_syscalls.a(exit.cpp.o): in function `sys_exit':
/home/libreliu/.conan/data/includeos/0.14.2-1208/includeos/latest/source/src/musl/exit.cpp:10: undefined reference to `os::print(char const*, unsigned long)'
make[2]: *** [CMakeFiles/hello.elf.bin.dir/build.make:117: bin/hello.elf.bin] Error 1
make[1]: *** [CMakeFiles/Makefile2:73: CMakeFiles/hello.elf.bin.dir/all] Error 2
make: *** [Makefile:84: all] Error 2
```