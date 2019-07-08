```
ELF Multiboot Bootloader
DTB Start=83770, End=8829d, Size=4b2d
Binary Start=882a0, End=1e0ea0, Size=158c00
Detected Multiboot header at 892a4, magic = 1badb002, flags = 10003, checksum = e4514ffb
[ MOVE ] dest=8000000, src=83770, size=4b2d
[ MOVE ] ELF_START=80000, _end=1e0f40
md_hdr_offset = 1004 (Hex)
hdr->flags[0] set, 4kb page aligned required
hdr->flags[1] set, avail mem struct required
hdr->flags[2] not set, video mode table not required
hdr->flags[16] set, extra fields valid
header_addr=201004, load_addr=2000e8, load_end_addr=2f50e0, bss_end_addr=2f50e0, entry_addr=201000
real_load=88388, real_load_end=17d380, real_bss_end=17d380, real_entry=892a0
[ MOVE ] dest=2000e8, src=88388, size=f4ff8
[ MOVE ] ELF_START=80000, _end=1e0f40
move ok.
bss cleared
ZT debugging
Enter kprintf.
Magic %zx addrin %zx
Enter kprintf.
CurrentEL 00000001

size_cells : 00000001

addr_cells : 00000001

mem_offset : 000034A4

RAM BASE : 0000000000000000

RAM SIZE : 0000000040000000

Pre_BSS
Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION

Enter kprintf.
SERROR EXCEPTION
```

```
ELF Multiboot Bootloader
DTB Start=83770, End=8829d, Size=4b2d
Binary Start=882a0, End=1e0ea0, Size=158c00
Detected Multiboot header at 892a4, magic = 1badb002, flags = 10003, checksum = e4514ffb
[ MOVE ] dest=8000000, src=83770, size=4b2d
[ MOVE ] ELF_START=80000, _end=1e0f40
md_hdr_offset = 1004 (Hex)
hdr->flags[0] set, 4kb page aligned required
hdr->flags[1] set, avail mem struct required
hdr->flags[2] not set, video mode table not required
hdr->flags[16] set, extra fields valid
header_addr=201004, load_addr=2000e8, load_end_addr=2f50e0, bss_end_addr=2f50e0, entry_addr=201000
real_load=88388, real_load_end=17d380, real_bss_end=17d380, real_entry=892a0
[ MOVE ] dest=2000e8, src=88388, size=f4ff8
[ MOVE ] ELF_START=80000, _end=1e0f40
move ok.
bss cleared
ZT debugging
Enter kprintf.
Magic %zx addrin %zx
Enter kprintf.
CurrentEL 00000001

size_cells : 00000001

addr_cells : 00000001

mem_offset : 000034A4

RAM BASE : 0000000000000000

RAM SIZE : 0000000040000000

Pre_BSS
Enter kprintf.
BSS_START=2e8000, BSS_END=2f50e0
Enter kprintf.
Clearing address: 2e8000
Enter kprintf.
Clearing address: 2e8001
Enter kprintf.
Clearing address: 2e8002
Enter kprintf.
Clearing address: 2e8003
Enter kprintf.
Clearing address: 2e8004
Enter kprintf.
Clearing address: 2e8005
Enter kprintf.
Clearing address: 2e8006
Enter kprintf.
Clearing address: 2e8007
Enter kprintf.
Clearing address: 2e8008
（......）
Clearing address: 2f50da
Enter kprintf.
Clearing address: 2f50db
Enter kprintf.
Clearing address: 2f50dc
Enter kprintf.
Clearing address: 2f50dd
Enter kprintf.
Clearing address: 2f50de
Enter kprintf.
Clearing address: 2f50df
Enter kprintf.
Clearing address: 2f50e0
Instantiate machine
Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION

Enter kprintf.
FIQ EXCEPTION
```