#ifndef MULTIBOOT_H_INCLUDED
#define MULTIBOOT_H_INCLUDED

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} multiboot_hdr;

int multiboot_search(char *);
int multiboot_boot(char *);

#endif