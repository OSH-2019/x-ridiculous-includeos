/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "gpio.h" // get MMIO_BASE
// #include "uart.h"
// #include <stdint.h>

typedef unsigned long uint64_t;

#define PAGESIZE 4096

// granularity
#define PT_PAGE 0b11  // 4k granule
#define PT_BLOCK 0b01 // 2M granule
// accessibility
#define PT_KERNEL (0 << 6) // privileged, supervisor EL1 access only
#define PT_USER (1 << 6)   // unprivileged, EL0 access allowed
#define PT_RW (0 << 7)     // read-write
#define PT_RO (1 << 7)     // read-only
#define PT_AF (1 << 10)    // accessed flag
#define PT_NX (1UL << 54)  // no execute
// shareability
#define PT_OSH (2 << 8) // outter shareable
#define PT_ISH (3 << 8) // inner shareable
// defined in MAIR register
#define PT_MEM (0 << 2) // normal memory
#define PT_DEV (1 << 2) // device MMIO
#define PT_NC (2 << 2)  // non-cachable

#define TTBR_CNP 1

// get addresses from linker
// extern volatile unsigned char _data;
// extern volatile unsigned char _end;

// extern volatile unsigned char __text_start;
// extern volatile unsigned char __text_end;

extern volatile unsigned char _TEXT_START_;
extern volatile unsigned char _EXEC_END_;
extern volatile unsigned char _READONLY_START_;
extern volatile unsigned char _READONLY_END_;

/**
 * Set up page translation tables and enable virtual memory
 */
void mmu_init()
{
    // 页表基地址,在 mmio 前
    uint64_t page_addr = (uint64_t)MMIO_BASE - 0xF000000;
    uint64_t *page_base = (uint64_t *)page_addr;

    uint64_t text_start_page = (uint64_t)&_TEXT_START_ / PAGESIZE;
    uint64_t exec_end_page = (uint64_t)&_EXEC_END_ / PAGESIZE + ((uint64_t)&_EXEC_END_ % PAGESIZE == 0 ? 0 : 1);
    uint64_t ro_start_page = (uint64_t)&_READONLY_START_ / PAGESIZE;
    uint64_t ro_end_page = (uint64_t)&_READONLY_END_ / PAGESIZE + ((uint64_t)&_READONLY_END_ % PAGESIZE == 0 ? 0 : 1);

    // mmio 开始 页号
    uint64_t mmio_start_page = (uint64_t)MMIO_BASE / PAGESIZE;

    uint64_t page_entries = PAGESIZE / 8;

    // TTBR0, identity L1
    // L1 条目 只需要一个，覆盖 1G 空间
    page_base[0 * page_entries + 0] = (page_addr + 1 * PAGESIZE) | // physical address
                                      PT_PAGE |                    // it has the "Present" flag, which must be set, and we have area in it mapped by pages
                                      PT_AF |                      // accessed flag. Without this we're going to have a Data Abort exception
                                      PT_USER |                    // non-privileged
                                      PT_ISH |                     // inner shareable
                                      PT_MEM;                      // normal memory

    // uart_puts("start init L2\n");

    // uint64_t L2_entries = PAGESIZE / 8;

    // 512 个 L2 条目
    for (uint64_t i = 0; i < page_entries; i++)
    {
        page_base[1 * page_entries + i] = (page_addr + (2 + i) * PAGESIZE) | // physical address
                                          PT_PAGE |                          // it has the "Present" flag, which must be set, and we have area in it mapped by pages
                                          PT_AF |                            // accessed flag. Without this we're going to have a Data Abort exception
                                          PT_USER |                          // non-privileged
                                          PT_ISH |                           // inner shareable
                                          PT_MEM;                            // normal memory

        // 每个 L2 对应 512 个 L3
        for (uint64_t j = 0; j < page_entries; j++)
        {
            uint64_t page_index = i * page_entries + j;
            uint64_t entry = (page_index * PAGESIZE) | // physical address
                             PT_PAGE |                 // it has the "Present" flag, which must be set, and we have area in it mapped by pages
                             PT_AF |                   // accessed flag. Without this we're going to have a Data Abort exception
                             PT_USER;                  // non-privileged
            // executable RO
            if ((page_index >= text_start_page) && (page_index < exec_end_page))
            {
                entry |= (PT_RO | PT_ISH | PT_MEM);
            }
            // read only data
            else if ((page_index >= ro_start_page) && (page_index < ro_end_page))
            {
                entry |= (PT_RO | PT_NX | PT_ISH | PT_MEM);
            }
            // mmio Outter shared
            else if (page_index >= mmio_start_page)
            {
                entry |= (PT_NX | PT_RW | PT_OSH | PT_DEV);
            }
            // other RW
            else
            {
                entry |= (PT_NX | PT_RW | PT_ISH | PT_MEM);
            }
            page_base[2 * page_entries + page_index] = entry;
        }
    }

    // // 1G / 4K = 262144 个 4K 页
    // uint64_t L3_entries = (uint64_t)1 * 1024 * 1024 * 1024 / PAGESIZE;
    // for (uint64_t i = 0; i < L3_entries; i++)
    // {
    //     uint64_t entry = (i * PAGESIZE) | // physical address
    //                      PT_PAGE |        // it has the "Present" flag, which must be set, and we have area in it mapped by pages
    //                      PT_AF |          // accessed flag. Without this we're going to have a Data Abort exception
    //                      PT_USER;         // non-privileged
    //     // text 段 RO
    //     if ((i >= text_start_page) && (i < text_end_page))
    //     {
    //         entry |= (PT_RO | PT_ISH | PT_MEM);
    //     }
    //     // 设备内存
    //     else if (i >= mmio_start_page)
    //     {
    //         entry |= (PT_NX | PT_RW | PT_OSH | PT_DEV);
    //     }
    //     // 其他
    //     else
    //     {
    //         entry |= (PT_NX | PT_RW | PT_ISH | PT_MEM);
    //     }
    //     page_base[2 * page_entries + i] = entry;
    // }

    // // identity L2, first 2M block
    // page_base[2 * 512] = (unsigned long)((unsigned char *)&_end + 3 * PAGESIZE) | // physical address
    //                      PT_PAGE |                                                // we have area in it mapped by pages
    //                      PT_AF |                                                  // accessed flag
    //                      PT_USER |                                                // non-privileged
    //                      PT_ISH |                                                 // inner shareable
    //                      PT_MEM;                                                  // normal memory

    // // identity L2 2M blocks
    // b = MMIO_BASE >> 21;
    // // skip 0th, as we're about to map it by L3
    // for (r = 1; r < 512; r++)
    //     page_base[2 * 512 + r] = (unsigned long)((r << 21)) |                  // physical address
    //                              PT_BLOCK |                                    // map 2M block
    //                              PT_AF |                                       // accessed flag
    //                              PT_NX |                                       // no execute
    //                              PT_USER |                                     // non-privileged
    //                              (r >= b ? PT_OSH | PT_DEV : PT_ISH | PT_MEM); // different attributes for device memory

    // // identity L3
    // for (r = 0; r < 512; r++)
    //     page_base[3 * 512 + r] = (unsigned long)(r * PAGESIZE) |                         // physical address
    //                              PT_PAGE |                                               // map 4k
    //                              PT_AF |                                                 // accessed flag
    //                              PT_USER |                                               // non-privileged
    //                              PT_ISH |                                                // inner shareable
    //                              ((r < 0x80 || r >= data_page) ? PT_RW | PT_NX : PT_RO); // different for code and data

    uint64_t kernel_page_addr = page_addr - 3 * PAGESIZE;
    uint64_t *kernel_page_base = (uint64_t *)kernel_page_addr;

    // TTBR1, kernel L1
    kernel_page_base[0 * page_entries + 511] = (kernel_page_addr + 1 * PAGESIZE) | // physical address
                                               PT_PAGE |                           // we have area in it mapped by pages
                                               PT_AF |                             // accessed flag
                                               PT_KERNEL |                         // privileged
                                               PT_ISH |                            // inner shareable
                                               PT_MEM;                             // normal memory

    // kernel L2
    kernel_page_base[1 * page_entries + 511] = (kernel_page_addr + 2 * PAGESIZE) | // physical address
                                               PT_PAGE |                           // we have area in it mapped by pages
                                               PT_AF |                             // accessed flag
                                               PT_KERNEL |                         // privileged
                                               PT_ISH |                            // inner shareable
                                               PT_MEM;                             // normal memory

    // kernel L3
    kernel_page_base[2 * page_entries] = (uint64_t)(MMIO_BASE + 0x00201000) | // physical address
                                         PT_PAGE |                            // map 4k
                                         PT_AF |                              // accessed flag
                                         PT_NX |                              // no execute
                                         PT_KERNEL |                          // privileged
                                         PT_OSH |                             // outter shareable
                                         PT_DEV;                              // device memory

    /* okay, now we have to set system registers to enable MMU */
    uint64_t r, b;

    // check for 4k granule and at least 36 bits physical address bus */
    asm volatile("mrs %0, id_aa64mmfr0_el1"
                 : "=r"(r));
    b = r & 0xF;
    if (r & (0xF << 28) /*4k*/ || b < 1 /*36 bits*/)
    {
        // uart_puts("ERROR: 4k granule or 36 bit address space not supported\n");
        return;
    }

    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r = (0xFF << 0) | // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) | // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 << 16); // AttrIdx=2: non cacheable
    asm volatile("msr mair_el1, %0"
                 :
                 : "r"(r));

    // next, specify mapping characteristics in translate control register
    r = (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL << 23) |  // EPD1 enable higher half
        (25LL << 16) |   // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL << 7) |   // EPD0 enable lower half
        (25LL << 0);     // T0SZ=25, 3 levels (512G)
    asm volatile("msr tcr_el1, %0; isb"
                 :
                 : "r"(r));

    // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile("msr ttbr0_el1, %0"
                 :
                 : "r"(page_addr + TTBR_CNP));
    // upper half, kernel space
    asm volatile("msr ttbr1_el1, %0"
                 :
                 : "r"(kernel_page_addr + TTBR_CNP));

    // finally, toggle some bits in system control register to enable page translation
    asm volatile("dsb ish; isb; mrs %0, sctlr_el1"
                 : "=r"(r));
    r |= 0xC00800;     // set mandatory reserved bits
    r &= ~((1 << 25) | // clear EE, little endian translation tables
           (1 << 24) | // clear E0E
           (1 << 19) | // clear WXN
           (1 << 12) | // clear I, no instruction cache
           (1 << 4) |  // clear SA0
           (1 << 3) |  // clear SA
           (1 << 2) |  // clear C, no cache at all
           (1 << 1));  // clear A, no aligment check
    r |= (1 << 0);     // set M, enable MMU
    asm volatile("msr sctlr_el1, %0; isb"
                 :
                 : "r"(r));
}
