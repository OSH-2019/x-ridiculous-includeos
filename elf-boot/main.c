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

#include "uart.h"
#include "mbox.h"
#include "printf.h"
#include "move.h"

extern char _binary_bcm2837_rpi_3_b_plus_dtb_start;
extern char _binary_bcm2837_rpi_3_b_plus_dtb_end;
extern char *_binary_bcm2837_rpi_3_b_plus_dtb_size;

extern char _binary_hello_elf_bin_start;
extern char _binary_hello_elf_bin_end;
extern char _binary_hello_elf_bin_size;

#define DTB_START 0x8000000

extern char __bss_start;
extern char __bss_end;

extern int _end;
extern int _ELF_START_;

void mem_rand(void) {
    // Check for available area
    int rand_begin = &_end;
    int m = 20;
    for (char *i = rand_begin; i < 0x0fffffff; i++) {
        *i = m++;
    }
}


int main(void) {
    // Clear BSS
    for (char *p = &__bss_start; p <= &__bss_end; p++) {
        *p = 0;
    }

    //mem_rand();
    uart_init();

    printf("ELF Multiboot Bootloader\n");
    printf("DTB Start=%x, End=%x, Size=%x\n", 
            &_binary_bcm2837_rpi_3_b_plus_dtb_start, 
            &_binary_bcm2837_rpi_3_b_plus_dtb_end,
            &_binary_bcm2837_rpi_3_b_plus_dtb_size);
    printf("Binary Start=%x, End=%x, Size=%x\n", 
            &_binary_hello_elf_bin_start,
            &_binary_hello_elf_bin_end,
            &_binary_hello_elf_bin_size);
    if (!multiboot_search(&_binary_hello_elf_bin_start)) {
        move(DTB_START, &_binary_bcm2837_rpi_3_b_plus_dtb_start, &_binary_bcm2837_rpi_3_b_plus_dtb_size);
        multiboot_boot(&_binary_hello_elf_bin_start);
    }

}