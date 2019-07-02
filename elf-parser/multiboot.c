#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>

extern char *file_base;
extern int ei_class;

int mb_hdr_detected = 0;
// Offset in char
int mb_hdr_offset = 0;

typedef struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} multiboot_hdr;

// Search for multiboot header
int multiboot_search() {
    multiboot_hdr *hdr;
    for (int i = 0; i < 8192; i++) {
        // magics are little-endian
        hdr = (multiboot_hdr *) (&(file_base[i]));
        // Check
        if (hdr->magic == 0x1BADB002 && (hdr->magic + hdr->flags + hdr->checksum == 0)) {
            printf("Detected Multiboot header, magic = %x, flags = %x, checksum = %x\n", hdr->magic, hdr->flags, hdr->checksum);
            mb_hdr_detected = 1;
            mb_hdr_offset = i;
        }
    }

    if (!mb_hdr_detected) {
        printf("Multiboot header not detected.\n");
        return -1;
    } else {
        return 0;
    }
}

int multiboot_parse() {
    multiboot_hdr *hdr = (multiboot_hdr *) (&(file_base[mb_hdr_offset]));
    if ((hdr->flags & 0x00000001) != 0) {
        // page-align bit
        printf("hdr->flags[0] set, 4kb page aligned required\n");
    } else {
        printf("hdr->flags[0] not set, 4kb page aligned not required\n");
    }

    if ((hdr->flags & 0x00000002) != 0) {
        // If bit 1 in the ‘flags’ word is set, then information on 
        // available memory via at least the ‘mem_*’ fields of the 
        // Multiboot information structure 
        // (see Boot information format) must be included. If the 
        // boot loader is capable of passing a memory map 
        // (the ‘mmap_*’ fields) and one exists, then it may be 
        // included as well.
        printf("hdr->flags[1] set, avail mem struct required\n");
    } else {
        printf("hdr->flags[1] not set, avail mem struct not required\n");
    }

    if ((hdr->flags & 0x00000004) != 0) {
        printf("hdr->flags[2] set, video mode table required\n");
    } else {
        printf("hdr->flags[2] not set, video mode table not required\n");
    }

    if ((hdr->flags & 0x00010000) != 0) {
        printf("hdr->flags[16] set, extra fields valid\n");
    } else {
        printf("hdr->flags[16] not set, extra fields not valid\n");
    }
}