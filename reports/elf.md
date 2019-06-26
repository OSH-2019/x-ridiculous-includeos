# ELF 方面代码整理

## Q1. musl libc 要求何种 ELF 信息？
```c
    aux[i++].set_long(AT_PHENT, ehdr->e_phentsize);
    aux[i++].set_ptr(AT_PHDR, ((uint8_t*)ehdr) + ehdr->e_phoff);
    aux[i++].set_long(AT_PHNUM, ehdr->e_phnum);
```
AT_PHENT: The address of the program headers of the executable. 

AT_PHDR:  The size of program header entry.

AT_PHNUM: The number of program headers.

## Q2. musl libc 中此信息的用处


## Note: Threading in Linux
https://en.wikipedia.org/wiki/Native_POSIX_Thread_Library


