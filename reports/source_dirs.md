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
| `-- uplink             
|-- linux
|-- manual_build.sh
|-- mod
|-- seed
|-- src
|-- test
|-- test.sh
|-- test_ukvm.sh
|-- vmbuild
`-- vmrunner
```



## 外部项目
- Botan，一个密码学和 TLS 的现代 C++ 库。在 `cmake/botan.cmake` 有安装的 CMake 配置。
