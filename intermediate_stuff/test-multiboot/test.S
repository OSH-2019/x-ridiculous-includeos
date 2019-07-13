.global _start
.extern _MULTIBOOT_START_
.extern _LOAD_START_
.extern _LOAD_END_
.extern _end

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

.section ".multiboot"
// MUST BE ALIGNED!
.align 4
.word 0x1BADB002
.word 0x00010003
.word 0 - 0x1BADB002 - 0x00010003
.word _MULTIBOOT_START_
.word _LOAD_START_
.word _LOAD_END_
.word _end
.word _start

.extern main

.section ".text.boot"
_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b
2:  // cpu id == 0

    // set stack before our code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
//    ldr     x1, =__bss_start
//    ldr     w2, =__bss_size
//3:  cbz     w2, 4f
//    str     xzr, [x1], #8
//    sub     w2, w2, #1
//    cbnz    w2, 3b

    // jump to C code, should not return
4:  bl      main
    // for failsafe, halt this core too
    b       1b