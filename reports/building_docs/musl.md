# musl 移植相关
musl （读 *muscle*） 是 IncludeOS 用的 libc 库。

## 移植 musl
参考 [Porting - musl](https://wiki.musl-libc.org/porting.html) 中对 「or1k」 的移植：
```
-rw-r--r--	arch/or1k/atomic.h	122	
-rw-r--r--	arch/or1k/bits/alltypes.h.in	23	
-rw-r--r--	arch/or1k/bits/endian.h	1	
-rw-r--r--	arch/or1k/bits/errno.h	134	
-rw-r--r--	arch/or1k/bits/fcntl.h	39	
-rw-r--r--	arch/or1k/bits/fenv.h	10	
-rw-r--r--	arch/or1k/bits/float.h	17	
-rw-r--r--	arch/or1k/bits/io.h	0	
-rw-r--r--	arch/or1k/bits/ioctl.h	197	
-rw-r--r--	arch/or1k/bits/ipc.h	13	
-rw-r--r--	arch/or1k/bits/limits.h	7	
-rw-r--r--	arch/or1k/bits/mman.h	61	
-rw-r--r--	arch/or1k/bits/msg.h	15	
-rw-r--r--	arch/or1k/bits/posix.h	2	
-rw-r--r--	arch/or1k/bits/reg.h	3	
-rw-r--r--	arch/or1k/bits/resource.h	0	
-rw-r--r--	arch/or1k/bits/sem.h	11	
-rw-r--r--	arch/or1k/bits/setjmp.h	1	
-rw-r--r--	arch/or1k/bits/shm.h	27	
-rw-r--r--	arch/or1k/bits/signal.h	80	
-rw-r--r--	arch/or1k/bits/socket.h	15	
-rw-r--r--	arch/or1k/bits/stat.h	21	
-rw-r--r--	arch/or1k/bits/statfs.h	7	
-rw-r--r--	arch/or1k/bits/stdarg.h	4	
-rw-r--r--	arch/or1k/bits/stdint.h	20	
-rw-r--r--	arch/or1k/bits/syscall.h	519	
-rw-r--r--	arch/or1k/bits/termios.h	159	
-rw-r--r--	arch/or1k/bits/user.h	0	
-rw-r--r--	arch/or1k/crt_arch.h	11	
-rw-r--r--	arch/or1k/pthread_arch.h	17	
-rw-r--r--	arch/or1k/reloc.h	47	
-rw-r--r--	arch/or1k/syscall_arch.h	154	
-rwxr-xr-x	configure	1	
-rw-r--r--	crt/or1k/crti.s	11	
-rw-r--r--	crt/or1k/crtn.s	9	
-rw-r--r--	include/elf.h	38	
-rw-r--r--	src/internal/or1k/syscall.s	13	
-rw-r--r--	src/ldso/or1k/dlsym.s	5	
-rw-r--r--	src/ldso/or1k/start.s	34	
-rw-r--r--	src/setjmp/or1k/longjmp.s	25	
-rw-r--r--	src/setjmp/or1k/setjmp.s	24	
-rw-r--r--	src/signal/or1k/sigsetjmp.s	22	
-rw-r--r--	src/thread/or1k/__set_thread_area.s	6	
-rw-r--r--	src/thread/or1k/__unmapself.s	8	
-rw-r--r--	src/thread/or1k/clone.s	30	
-rw-r--r--	src/thread/or1k/syscall_cp.s	20	
```

### 搞 `std::cout` / `printf`
只要搞一下 `open` 等调用就可以了。
（在 IncludeOS repo 里面没有搜索到 write 的实现？）

> IncludeOS 没实现 dup/dup2。
