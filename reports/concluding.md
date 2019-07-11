# includeOS on ARM 结题报告

## 项目介绍

IncludeOS 是一个 C++ 的 Unikernel 实现，并可以在 bare-metal 上运行。IncludeOS 提供了丰富的用于网络编程的库，但是目前还不支持在 ARM 上运行。裸机运行的 IncludeOS 相较于 Linux 发行版拥有更快的启动速度，并且减少了进程切换等的无谓开销，代码审计面更小，安全性更高。现有的树莓派的 Unikernel 对网络的支持很弱。在 IoT 领域中，有许多应用场景对延迟的要求十分苛刻，对安全性要求很高。而本项目意在将 IncludeOS 移植到 ARM 上，这样对延迟敏感，安全性要求高的 IoT 应用场景会有很大帮助。

## 立项依据

传统的操作系统代码量很大，每一个操作系统会向用户提供完整的服务，不管用户是否真正会用到。所有的服务内容都预先构建和激活，这就使得被传统操作系统的代码审计量大，系统攻击面广，也就存在更多可能的安全隐患。

includeOS 等 Unikernel 使用更加复杂的构建系统来分析用户代码，只链接实际使用的代码，从而大大降低了代码审计量，也降低了系统安全风险，提高了系统的安全性。

我们选择 includeOS 的一个重要原因是它具有更好的网络性能并且占用更少的资源。参考文献的实验（见**参考文献1**）展示了 IncludeOS 相比于传统方式的优势。实验的主要内容是一个简单 DNS-server，在 IncludeOS 和 Linux 上测试相同的数据流。鉴于这个实验的目的只是验证操作系统造成的资源开销，所以并不用一个功能齐全的 DNS-server，只需两个操作系统都运行相同的 DNS-server。测试的内容是 DNS 协议的部分实现，允许服务从 nslookup 和 dig 等工具回答实际的DNS查询，但仅限于 A-record。

## 项目进展

### 完成的预定任务目标

- 成功完成了系统构建

- 增添了 UART 驱动支持

- 增添了 GPIO 驱动支持

- 增添了 Frame Buffer 驱动支持

- 增添了 eMMC & SD Card 驱动支持

- 增添了 Exception Handler

- 实现了 MMU

- 实现了 File System

### 还将继续探究的问题

- 未能在裸机上 boot

- 未添加 USB 驱动

- 未完成 Ethernet 驱动

- 尚未完成 includeOS 和传统操作系统网络性能对比

## 工作摘要

### includeOS 源代码结构

includeOS Source Architecture
![includeOS Source Architecture](pics/includeOS&#32;Source&#32;Architecture.png)

includeOS Runtime Architecture
![includeOS Runtime Architecture](pics/includeOS&#32;Runtime&#32;Architecture.png)

### 构建过程



### 驱动支持

#### UART



#### GPIO



#### Framebuffer(Support for Screen Device)



#### eMMC & SD Card



#### MMU



#### Expection handler



#### File system



## 未来展望

- 未来希望能够在裸机上成功启动，以便进一步对各驱动的完成情况进行检验；

- 希望能够成功添加 USB 驱动，因为 USB 驱动是 Ethernet 驱动的基础和前提；

- 希望能够提供 Ethernet 支持；

- 希望能够完成 includeOS 和传统操作系统网络性能对比。

- 如果有可能的话，Merge 进 includeOS 的官方 AArch64 的工作，为开源社区作出贡献。

## 参考文献

1. [Alfred Bratterud, Alf-Andre Walla, Harek Haugerud, Paal E. Engelstad, Kyrre Begnum,"IncludeOS: A minimal, resource efficient
unikernel for cloud services"](https://github.com/includeos/IncludeOS/blob/master/doc/papers/IncludeOS_IEEE_CloudCom2015_PREPRINT.pdf)