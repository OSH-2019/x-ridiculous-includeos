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


