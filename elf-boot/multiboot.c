#include "printf.h"
#include "move.h"

/*--
Offset	Type	Field Name	Note 
0	u32	magic	required 
4	u32	flags	required 
8	u32	checksum	required 
12	u32	header_addr	if flags[16] is set 
    Contains the address corresponding to the beginning of 
    the Multiboot header — the physical memory location at
    which the magic value is supposed to be loaded. This 
    field serves to synchronize the mapping between OS 
    image offsets and physical memory addresses. 
16	u32	load_addr	if flags[16] is set 
    Contains the physical address of the beginning of the 
    text segment. The offset in the OS image file at which 
    to start loading is defined by the offset at which the
    header was found, minus (header_addr - load_addr). 
    load_addr must be less than or equal to header_addr. 
20	u32	load_end_addr	if flags[16] is set
    Contains the physical address of the end of the data 
    segment. (load_end_addr - load_addr) specifies how 
    much data to load. This implies that the text and data
    segments must be consecutive in the OS image; this 
    is true for existing a.out executable formats. If 
    this field is zero, the boot loader assumes that 
    the text and data segments occupy the whole OS image 
    file. 
24	u32	bss_end_addr	if flags[16] is set
    Contains the physical address of the end of the bss 
    segment. The boot loader initializes this area to 
    zero, and reserves the memory it occupies to avoid 
    placing boot modules and other data relevant to the 
    operating system in that area. If this field is zero
    , the boot loader assumes that no bss segment is present. 
28	u32	entry_addr	if flags[16] is set
    The physical address to which the boot loader should 
    jump in order to start running the operating system.
32	u32	mode_type	if flags[2] is set 
36	u32	width	if flags[2] is set 
40	u32	height	if flags[2] is set 
44	u32	depth	if flags[2] is set 
*/

#include "multiboot.h"

int mb_hdr_detected = 0;
// Offset in char
int mb_hdr_offset = 0;

multiboot_hdr *hdr;

// Search for multiboot header
int multiboot_search(char *file_base) {
    // Search 800KB, to deal with increasing IncludeOS binary size
    for (int i = 0; i < 819200; i++) {
        // magics are little-endian
        // Make sure we have aligned access
        hdr = (multiboot_hdr *)(((uint32_t *) (file_base)) + i);
        // hdr = (multiboot_hdr *)((file_base) + i);
        // Check
        if (hdr->magic == 0x1BADB002 && (hdr->magic + hdr->flags + hdr->checksum == 0)) {
            printf("Detected Multiboot header at %x, magic = %x, flags = %x, checksum = %x\n", hdr, hdr->magic, hdr->flags, hdr->checksum);
            mb_hdr_detected = 1;
            mb_hdr_offset = i * 4;
            // mb_hdr_offset = i;
            break;
        }
    }

    if (!mb_hdr_detected) {
        printf("Multiboot header not detected.\n");
        return -1;
    } else {
        return 0;
    }
}

void multiboot_explain(multiboot_hdr *h) {
    if ((h->flags & 0x00000001) != 0) {
        // page-align bit
        printf("hdr->flags[0] set, 4kb page aligned required\n");
    } else {
        printf("hdr->flags[0] not set, 4kb page aligned not required\n");
    }

    if ((h->flags & 0x00000002) != 0) {
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

    if ((h->flags & 0x00000004) != 0) {
        printf("hdr->flags[2] set, video mode table required\n");
    } else {
        printf("hdr->flags[2] not set, video mode table not required\n");
    }

    if ((h->flags & 0x00010000) != 0) {
        printf("hdr->flags[16] set, extra fields valid\n");
        printf("header_addr=%x, load_addr=%x, load_end_addr=%x, bss_end_addr=%x, entry_addr=%x\n",
            h->header_addr, h->load_addr, h->load_end_addr, h->bss_end_addr, h->entry_addr);
    } else {
        printf("hdr->flags[16] not set, extra fields not valid\n");
    }
} 

int multiboot_boot(char *file_base) {
    printf("md_hdr_offset = %x (Hex)\n", mb_hdr_offset);
    multiboot_explain(hdr);

    if (!(hdr->flags & 0x00010000)) {
        printf("No direct ELF parse support for now, abort.\n");
        return -1;
    }

    // Notice: Implicit conversion from uint32 to uint64 can be buggy with negative numbers
    // So convert them in advance, then substract
    char *real_load = (char *)((uint64_t) hdr + ((uint64_t)(hdr->load_addr) - (uint64_t)(hdr->header_addr)));
    char *real_load_end = (char *)((uint64_t) hdr + ((uint64_t)(hdr->load_end_addr) - (uint64_t)(hdr->header_addr)));
    char *real_bss_end = (char *)((uint64_t) hdr + ((uint64_t)(hdr->bss_end_addr) - (uint64_t)(hdr->header_addr)));
    void (*entry)(void) = (void(*)(void)) hdr->entry_addr;
    char *real_entry = (char *)((uint64_t) hdr + ((uint64_t)(hdr->entry_addr) - (uint64_t)(hdr->header_addr)));


    printf("real_load=%lx, real_load_end=%lx, real_bss_end=%lx, real_entry=%lx\n",
        real_load, real_load_end, real_bss_end, real_entry);
    move(hdr->load_addr, real_load, hdr->load_end_addr - hdr->load_addr);
    printf("move ok.\n");
    // Clear BSS
    for (char *p = real_load_end + 1; p <= real_bss_end; p++) {
        *p = 0;
    }
    printf("bss cleared\n");
    // TODO Set magic
    // Jump to entry

    entry();
    // Should never reach this
    return -1;
}