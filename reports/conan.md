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