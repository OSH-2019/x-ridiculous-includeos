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

#### GPIO

GPIO 驱动本质上是对特定内存读写任务的封装。\
接口封装了如下 API：

``` C
/* Read 32 bit value from a peripheral address. */
__uint32_t read_peri(volatile __uint32_t *paddr);
namespace hw
{
	class GPIO
	{
		public:
			/* Read 32 bit value from a peripheral address. */
			__uint32_t read_peri(volatile __uint32_t *paddr);

			/*  Write 32 bit value to a peripheral address. */ 
			void write_peri(volatile __uint32_t *paddr, __uint32_t value);

			/* Set bits in the address. */
			void gpio_set_bits(volatile __uint32_t *paddr, __uint32_t mask, __uint32_t value);

			/* Select the function of GPIO. */
			void gpio_func_select(const __uint8_t pin, __uint8_t mode);

			/* Set output pin. */
			void gpio_set(const __uint8_t pin);

			/* Clear output pin. */
			void gpio_clr(const __uint8_t pin);

			/* Read the current level of input pin. */
			int gpio_read_level(const __uint8_t pin);

			/* Get the time. */
			__uint32_t gpio_timer(void);
			
			/* Delay count. Return 0 if overflow, return 1 if success. */
			int gpio_delay(int count);
	};
};
```

API 封装在 `hw` 命名空间下。\
利用提供的 API 可以轻松对 GPIO 端口进行读写操作。

#### UART

实现 UART 驱动需要用到 Mailbox 通信。

##### Mailbox

Mailbox 是 CPU 与 VideoCore 通信的工具，VideoCore 是 Broadcom 开发的非传统意义的 GPU。树莓派的启动就是由 VideoCore 负责的，它还负责管理外设。所以要实现 UART 驱动，需要通过 Mailbox 与 VideoCore 通信。

Mailbox 有以下几个可用的channels，不同的channel，消息格式不同

- 0: Power management
- 1: Framebuffer
- 2: Virtual UART
- 3: VCHIQ
- 4: LEDs
- 5: Buttons
- 6: Touch screen
- 7:
- 8: Property tags (ARM -> VC)
- 9: Property tags (VC -> ARM)

本次实现中主要使用 channel 8.

Channel 8 的消息格式如下：
```text
       0       4       8      12      16      20      24      28      32
       +---------------------------------------------------------------+
0x00   |                         Buffer Size(消息缓冲区长度)                           |
       +---------------------------------------------------------------+
0x04   |                   Request(0)/Response(0x80000000 for success, 0x80000001 for an error) Code                       |
       +---------------------------------------------------------------+
0x08   |                             Tags(具体命令)                              |
...    \\                                                             \\
0xXX   |                             Tags                              |
       +---------------------------------------------------------------+
0xXX+4 |                           End Tag (0，表示消息结束)                         |
       +---------------------------------------------------------------+
0xXX+8 |                           Padding                             |
...    \\                                                             \\
0xXX+16|                           Padding                             |
       +---------------------------------------------------------------+
```

其中 Tag 的格式如下：

```text
       0       4       8      12      16      20      24      28      32
       +---------------------------------------------------------------+
0x00   |                         Tag Identity                          |
       +---------------------------------------------------------------+
0x04   |                       Value Buffer Size                       |
       +---------------------------------------------------------------+
0x08   |                     Request/Response Code                     |
       +---------------------------------------------------------------+
0x0C   |                          Value Buffer                         |
...    \\                                                             \\
0xXX   | Value Buffer  |                    Padding                    |
       +---------------------------------------------------------------+
```

  - Tag Identity 一般格式为0x000XYZZZ，X 表示要传递消息的硬件设备, Y 表示命令的类别(0 if get, 4 = test, 8 if set) and ZZZ 描述具体的命令；

  - Value Buffer Size 为 Tag 的缓冲区长度；

  - Request(0)/Response(0x80000000+length of results) Code;

  - Value Buffer 存储请求的具体参数或者相应的结果。
  
UART 的发送和接收引脚分别位于 GPIO 14 和 GPIO 15.\
为了完成 UART 驱动支持，需要：

- 使用 Mailbox 和 VideoCore 通信设置时钟频率；

- 设置 GPIO 14 和 GPIO 15 的 function 为 alt0；

- 设置 pull-up/down resistor（使信号更加稳定，减少外界信号干扰）；

- 设置 UART 传输参数（波特率、校验位等）。

最终提供了如下接口：
```C
/* Init UART. */
void uart_init();

/* Send to UART. */
void uart_send(unsigned int c);

/* Get from UART. */
char uart_getc();

/* Send a string to UART. */
void uart_puts(char *s);
```

#### Framebuffer(Support for Screen Device)

Framebuffer 是 Linux 系统抽象给用户读写显示设备的接口。它把消除了显示设备的底层硬件差异，将底层抽象为一块缓冲区，使得上层应用可以直接对缓冲区进行读写即可完成对屏幕的读写。

驱动的编写过程有些部分可以参考使用 Verilog 编写 VGA 驱动的过程。

这里需要使用字模文件，这里使用 Linux Console 字体文件作为字模。

提供了如下 API：

```C
/* Initialize framebuffer. */
int screen_init();

/* Print string on the screen */
void screen_print(const char *s);

```

- `screen_init`初始化`screen device`。它设定通过 Mailbox 设定屏幕分辨率等信息，初始化起始打印位置（默认 (0,0) ），并且获得 framebuffer 的起始地址；

- `screen_print`函数会根据字模在屏幕上打印字符串，即向对应的 framebuffer 里存放数据。

API 封装在 `hw` 目录下。

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

2. [Rasberry pi Mailbox prop-channel](https://jsandler18.github.io/extra/prop-channel.html)