/* 
   SEGMENT_START(segment, default)
        Return the base address of the named segment. If an explicit 
        value has already been given for this segment (with a 
        command-line ‘-T’ option) then that value will be returned 
        otherwise the value will be default. At present, the ‘-T’ 
        command-line option can only be used to set the base address 
        for the “text”, “data”, and “bss” sections, but you can use 
        SEGMENT_START with any segment name.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x90000 + SIZEOF_HEADERS;
    PROVIDE ( _MULTIBOOT_START_ = . );
    PROVIDE ( _LOAD_START_ = . );
    /* No load section ".multiboot" */
    /*.multiboot : {
        *(.multiboot)
    }*/


    /* KEEP is used to prevent unused input section from being garbage collected */
    /* .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) } */
    .text : { *(.multiboot) *(.text.boot) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }

    PROVIDE ( _LOAD_END_ = . );

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*)}
}