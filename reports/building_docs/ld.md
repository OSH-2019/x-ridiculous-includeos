# LinkerScript

最简单的 Linker script：
```
SECTIONS
{
    . = 0x80000;
    .text : { *(.text.boot) }

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
```

更详细的内容请参见 https://wen00072.github.io/blog/2014/03/14/study-on-the-linker-script/