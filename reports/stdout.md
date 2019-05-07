# stdout 在哪里

> 当在 IncludeOS Service 中调用 printf 时，究竟发生了什么？

## 全局搜索之可能有关联的文件

### `__printf_chk`
有一个这样的实现，在 `src/crt/c_abi.c`。

但是最后都调用 `done = vfprintf (fp, format, ap);`。

没有找到 vfprintf 的实现。
> 应该在 musl-libc 里面？在不在 newlib 里面？

### `kprintf`
Eg. `src\platform\x86_solo5\serial1.cpp`:
```c
void kprintf(const char* format, ...)
{
  char buf[8192];
  va_list aptr;
  va_start(aptr, format);
  vsnprintf(buf, sizeof(buf), format, aptr);
  __serial_print1(buf);
  va_end(aptr);
}
```


### 搜索 `stdout`
[Github Search - stdout](https://github.com/includeos/IncludeOS/search?q=stdout&unscoped_q=stdout)

- linker.ld
```
  /* Stdout g constructors */
  .stdout_ctors :
  {
    PROVIDE_HIDDEN (__stdout_ctors_start = .);
    */x86_64/drivers/stdout/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__stdout_ctors_end = .);
  }
```

- drivers/CMakeLists.txt
```
# make LiveUpdate visible to drivers
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)

# Simple stuff
add_library(boot_logger STATIC stdout/bootlog.cpp)
add_library(default_stdout STATIC "stdout/default_stdout.cpp")

add_library(ide_readwrite STATIC ide.cpp)
```

- x86_nano/platform.cpp
```
// not supported!
void OS::block() {}

// default to serial
void OS::default_stdout(const char* str, const size_t len)
int SMP::cpu_count() noexcept { return 1; }

void OS::halt() {
  asm("hlt");
}

// default stdout/logging states
__attribute__((weak))
```

## 从简单服务反编译
`ZtSimpleApp` 是一个简单的 `hello_world` 例程魔改。

构建后会生成符合 Multiboot 规范的 ELF64 (amd64) binary。

用 `objdump --disassemble ZtSimpleApp > dump.d` 生成的结果如下：

```
00000000003aad8c <vfprintf>:
  3aad8c:	55                   	push   %rbp
  3aad8d:	41 57                	push   %r15
  3aad8f:	41 56                	push   %r14
  3aad91:	41 55                	push   %r13
  3aad93:	41 54                	push   %r12
  3aad95:	53                   	push   %rbx
  3aad96:	48 81 ec 48 01 00 00 	sub    $0x148,%rsp
  3aad9d:	49 89 f7             	mov    %rsi,%r15
  3aada0:	48 89 fb             	mov    %rdi,%rbx
  3aada3:	0f 57 c0             	xorps  %xmm0,%xmm0
  3aada6:	4c 8d 44 24 20       	lea    0x20(%rsp),%r8
  3aadab:	41 0f 29 40 10       	movaps %xmm0,0x10(%r8)
  3aadb0:	41 0f 29 00          	movaps %xmm0,(%r8)
  3aadb4:	49 c7 40 20 00 00 00 	movq   $0x0,0x20(%r8)
  3aadbb:	00 
  3aadbc:	48 8b 4a 10          	mov    0x10(%rdx),%rcx
  3aadc0:	48 89 e0             	mov    %rsp,%rax
  3aadc3:	48 89 48 10          	mov    %rcx,0x10(%rax)
  3aadc7:	0f 10 02             	movups (%rdx),%xmm0
  3aadca:	0f 29 00             	movaps %xmm0,(%rax)
  3aadcd:	45 31 f6             	xor    %r14d,%r14d
  3aadd0:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
  3aadd7:	00 
  3aadd8:	31 ff                	xor    %edi,%edi
  3aadda:	48 89 c2             	mov    %rax,%rdx
  3aaddd:	e8 13 01 00 00       	callq  3aaef5 <printf_core>
  3aade2:	85 c0                	test   %eax,%eax
  3aade4:	78 56                	js     3aae3c <vfprintf+0xb0>
  3aade6:	83 bb 8c 00 00 00 00 	cmpl   $0x0,0x8c(%rbx)
  3aaded:	78 0b                	js     3aadfa <vfprintf+0x6e>
  3aadef:	48 89 df             	mov    %rbx,%rdi
  3aadf2:	e8 c0 70 00 00       	callq  3b1eb7 <__lockfile>
  3aadf7:	41 89 c6             	mov    %eax,%r14d
  3aadfa:	44 8b 23             	mov    (%rbx),%r12d
  3aadfd:	80 bb 8a 00 00 00 00 	cmpb   $0x0,0x8a(%rbx)
  3aae04:	7f 08                	jg     3aae0e <vfprintf+0x82>
  3aae06:	44 89 e0             	mov    %r12d,%eax
  3aae09:	83 e0 df             	and    $0xffffffdf,%eax
  3aae0c:	89 03                	mov    %eax,(%rbx)
  3aae0e:	41 83 e4 20          	and    $0x20,%r12d
  3aae12:	48 83 7b 60 00       	cmpq   $0x0,0x60(%rbx)
  3aae17:	74 2d                	je     3aae46 <vfprintf+0xba>
  3aae19:	48 89 e2             	mov    %rsp,%rdx
  3aae1c:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
  3aae23:	00 
  3aae24:	4c 8d 44 24 20       	lea    0x20(%rsp),%r8
  3aae29:	48 89 df             	mov    %rbx,%rdi
  3aae2c:	4c 89 fe             	mov    %r15,%rsi
  3aae2f:	e8 c1 00 00 00       	callq  3aaef5 <printf_core>
  3aae34:	41 89 c7             	mov    %eax,%r15d
  3aae37:	e9 81 00 00 00       	jmpq   3aaebd <vfprintf+0x131>
  3aae3c:	bd ff ff ff ff       	mov    $0xffffffff,%ebp
  3aae41:	e9 9b 00 00 00       	jmpq   3aaee1 <vfprintf+0x155>
  3aae46:	48 8b 6b 58          	mov    0x58(%rbx),%rbp
  3aae4a:	48 8d 44 24 50       	lea    0x50(%rsp),%rax
  3aae4f:	48 89 43 58          	mov    %rax,0x58(%rbx)
  3aae53:	48 89 43 38          	mov    %rax,0x38(%rbx)
  3aae57:	48 89 43 28          	mov    %rax,0x28(%rbx)
  3aae5b:	b8 50 00 00 00       	mov    $0x50,%eax
  3aae60:	48 89 43 60          	mov    %rax,0x60(%rbx)
  3aae64:	48 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%rax
  3aae6b:	00 
  3aae6c:	48 89 43 20          	mov    %rax,0x20(%rbx)
  3aae70:	48 89 e2             	mov    %rsp,%rdx
  3aae73:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
  3aae7a:	00 
  3aae7b:	4c 8d 44 24 20       	lea    0x20(%rsp),%r8
  3aae80:	48 89 df             	mov    %rbx,%rdi
  3aae83:	4c 89 fe             	mov    %r15,%rsi
  3aae86:	e8 6a 00 00 00       	callq  3aaef5 <printf_core>
  3aae8b:	41 89 c7             	mov    %eax,%r15d
  3aae8e:	48 85 ed             	test   %rbp,%rbp
  3aae91:	74 2a                	je     3aaebd <vfprintf+0x131>
  3aae93:	45 31 ed             	xor    %r13d,%r13d
  3aae96:	31 f6                	xor    %esi,%esi
  3aae98:	31 d2                	xor    %edx,%edx
  3aae9a:	48 89 df             	mov    %rbx,%rdi
  3aae9d:	ff 53 48             	callq  *0x48(%rbx)
  3aaea0:	48 83 7b 28 01       	cmpq   $0x1,0x28(%rbx)
  3aaea5:	19 c0                	sbb    %eax,%eax
  3aaea7:	41 09 c7             	or     %eax,%r15d
  3aaeaa:	48 89 6b 58          	mov    %rbp,0x58(%rbx)
  3aaeae:	4c 89 6b 60          	mov    %r13,0x60(%rbx)
  3aaeb2:	4c 89 6b 38          	mov    %r13,0x38(%rbx)
  3aaeb6:	0f 57 c0             	xorps  %xmm0,%xmm0
  3aaeb9:	0f 11 43 20          	movups %xmm0,0x20(%rbx)
  3aaebd:	8b 03                	mov    (%rbx),%eax
  3aaebf:	89 c1                	mov    %eax,%ecx
  3aaec1:	83 e1 20             	and    $0x20,%ecx
  3aaec4:	83 f9 01             	cmp    $0x1,%ecx
  3aaec7:	19 ed                	sbb    %ebp,%ebp
  3aaec9:	f7 d5                	not    %ebp
  3aaecb:	41 09 c4             	or     %eax,%r12d
  3aaece:	44 89 23             	mov    %r12d,(%rbx)
  3aaed1:	45 85 f6             	test   %r14d,%r14d
  3aaed4:	74 08                	je     3aaede <vfprintf+0x152>
  3aaed6:	48 89 df             	mov    %rbx,%rdi
  3aaed9:	e8 4d 70 00 00       	callq  3b1f2b <__unlockfile>
  3aaede:	44 09 fd             	or     %r15d,%ebp
  3aaee1:	89 e8                	mov    %ebp,%eax
  3aaee3:	48 81 c4 48 01 00 00 	add    $0x148,%rsp
  3aaeea:	5b                   	pop    %rbx
  3aaeeb:	41 5c                	pop    %r12
  3aaeed:	41 5d                	pop    %r13
  3aaeef:	41 5e                	pop    %r14
  3aaef1:	41 5f                	pop    %r15
  3aaef3:	5d                   	pop    %rbp
  3aaef4:	c3                   	retq   
```

可以看到，`vfprintf` 调用 `printf_core`，说明这段代码就是 musl-libc 中的[这段代码](https://git.musl-libc.org/cgit/musl/tree/src/stdio/vfprintf.c)。

下面的`__unlockfile()` 是 `FUNLOCK` 宏（参上面链接的源码），可能是多线程环境下（因为 musl 集成了 lm , lpthread 等库）的一个宏。

继续挖掘，`printf_core` 调用 `__fwritex` 函数。

在 musl 的 [`src/internal/stdio_impl.h`](https://git.musl-libc.org/cgit/musl/tree/src/internal/stdio_impl.h) 里面有 `__fwritex` 的声明：`size_t __fwritex(const unsigned char *, size_t, FILE *);`

在 [`src/stdio/fwrite.c`](https://git.musl-libc.org/cgit/musl/tree/src/stdio/fwrite.c) 有 `__fwritex` 的实现：
```c
size_t __fwritex(const unsigned char *restrict s, size_t l, FILE *restrict f)
{
	size_t i=0;

	if (!f->wend && __towrite(f)) return 0;

	if (l > f->wend - f->wpos) return f->write(f, s, l);

	if (f->lbf >= 0) {
		/* Match /^(.*\n|)/ */
		for (i=l; i && s[i-1] != '\n'; i--);
		if (i) {
			size_t n = f->write(f, s, i);
			if (n < i) return n;
			s += i;
			l -= i;
		}
	}

	memcpy(f->wpos, s, l);
	f->wpos += l;
	return l+i;
}
```

可以看到它会调用 FILE 结构体的 `write` 函数指针。那么这个指针是被谁初始化的呢？

> FILE *stdout 应该是全局的变量，可以直接在 `kernel_start` 等处赋初值（？）

在 [`src/stdio/stdout.c`](https://git.musl-libc.org/cgit/musl/tree/src/stdio/stdout.c) 可以看到如下：
```c
hidden FILE __stdout_FILE = {
	.buf = buf+UNGET,
	.buf_size = sizeof buf-UNGET,
	.fd = 1,
	.flags = F_PERM | F_NORD,
	.lbf = '\n',
	.write = __stdout_write,
	.seek = __stdio_seek,
	.close = __stdio_close,
	.lock = -1,
};
FILE *const stdout = &__stdout_FILE;
FILE *volatile __stdout_used = &__stdout_FILE;
```

所以输出方法是 `__stdout_write`，在 `src/stdio/__stdout_write.c`：
```c
#include "stdio_impl.h"
#include <sys/ioctl.h>

size_t __stdout_write(FILE *f, const unsigned char *buf, size_t len)
{
	struct winsize wsz;
	f->write = __stdio_write;
	if (!(f->flags & F_SVB) && __syscall(SYS_ioctl, f->fd, TIOCGWINSZ, &wsz))
		f->lbf = -1;
	return __stdio_write(f, buf, len);
}
```
然后 `__stdio_write` 在 `src/stdio/__stdio_write.c`：
```c
#include "stdio_impl.h"
#include <sys/uio.h>

size_t __stdio_write(FILE *f, const unsigned char *buf, size_t len)
{
	struct iovec iovs[2] = {
		{ .iov_base = f->wbase, .iov_len = f->wpos-f->wbase },
		{ .iov_base = (void *)buf, .iov_len = len }
	};
	struct iovec *iov = iovs;
	size_t rem = iov[0].iov_len + iov[1].iov_len;
	int iovcnt = 2;
	ssize_t cnt;
	for (;;) {
		cnt = syscall(SYS_writev, f->fd, iov, iovcnt);
		if (cnt == rem) {
			f->wend = f->buf + f->buf_size;
			f->wpos = f->wbase = f->buf;
			return len;
		}
		if (cnt < 0) {
			f->wpos = f->wbase = f->wend = 0;
			f->flags |= F_ERR;
			return iovcnt == 2 ? 0 : len-iov[0].iov_len;
		}
		rem -= cnt;
		if (cnt > iov[0].iov_len) {
			cnt -= iov[0].iov_len;
			iov++; iovcnt--;
		}
		iov[0].iov_base = (char *)iov[0].iov_base + cnt;
		iov[0].iov_len -= cnt;
	}
}
```

可以看到用的是 `syscall(SYS_writev, f->fd, iov, iovcnt)`，就会调用 `src/misc/syscall.c` 里面的函数 `syscall`，然后是 `arch/` 相关的那些函数内联汇编的 `__syscall` 了。但是 `IncludeOS` 真的实现了那些 syscall 吗？

没有，IncludeOS 把它们 patch 掉了：（参[`etc/musl/musl_full.patch` @ IncludeOS master](https://github.com/includeos/IncludeOS/blob/master/etc/musl/musl_full.patch)）

节选如下：（还有 pthread 那里的 `syscall` 也 patch 掉了，没截）
```diff
--- a/src/internal/syscall.h
+++ b/src/internal/syscall.h
@@ -2,7 +2,15 @@
 #define _INTERNAL_SYSCALL_H
 
 #include <sys/syscall.h>
-#include "syscall_arch.h"
+//#include "syscall_arch.h"y
+#include "includeos_syscalls.h"
+
+
+#define __SYSCALL_LL_E(x)				\
+((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
+((union { long long ll; long l[2]; }){ .ll = x }).l[1]
+#define __SYSCALL_LL_O(x) __SYSCALL_LL_E((x))
+
 
 #ifndef SYSCALL_RLIM_INFINITY
 #define SYSCALL_RLIM_INFINITY (~0ULL)
@@ -26,56 +34,23 @@ long __syscall_ret(unsigned long), __syscall(syscall_arg_t, ...),
 	__syscall_cp(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t,
 	             syscall_arg_t, syscall_arg_t, syscall_arg_t);
 
-#ifdef SYSCALL_NO_INLINE
-#define __syscall0(n) (__syscall)(n)
-#define __syscall1(n,a) (__syscall)(n,__scc(a))
-#define __syscall2(n,a,b) (__syscall)(n,__scc(a),__scc(b))
-#define __syscall3(n,a,b,c) (__syscall)(n,__scc(a),__scc(b),__scc(c))
-#define __syscall4(n,a,b,c,d) (__syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d))
-#define __syscall5(n,a,b,c,d,e) (__syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e))
-#define __syscall6(n,a,b,c,d,e,f) (__syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),__scc(f))
-#else
-#define __syscall1(n,a) __syscall1(n,__scc(a))
-#define __syscall2(n,a,b) __syscall2(n,__scc(a),__scc(b))
-#define __syscall3(n,a,b,c) __syscall3(n,__scc(a),__scc(b),__scc(c))
-#define __syscall4(n,a,b,c,d) __syscall4(n,__scc(a),__scc(b),__scc(c),__scc(d))
-#define __syscall5(n,a,b,c,d,e) __syscall5(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e))
-#define __syscall6(n,a,b,c,d,e,f) __syscall6(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),__scc(f))
-#endif
-#define __syscall7(n,a,b,c,d,e,f,g) (__syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),__scc(f),__scc(g))
-
 #define __SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
 #define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)
 #define __SYSCALL_CONCAT_X(a,b) a##b
 #define __SYSCALL_CONCAT(a,b) __SYSCALL_CONCAT_X(a,b)
-#define __SYSCALL_DISP(b,...) __SYSCALL_CONCAT(b,__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)
+#define __syscall(a,...) syscall_##a(__VA_ARGS__)
+#define syscall(a,...) __syscall_ret(syscall_##a(__VA_ARGS__))
 
-#define __syscall(...) __SYSCALL_DISP(__syscall,__VA_ARGS__)
-#define syscall(...) __syscall_ret(__syscall(__VA_ARGS__))
 
 #define socketcall __socketcall
 #define socketcall_cp __socketcall_cp
 
-#define __syscall_cp0(n) (__syscall_cp)(n,0,0,0,0,0,0)
-#define __syscall_cp1(n,a) (__syscall_cp)(n,__scc(a),0,0,0,0,0)
-#define __syscall_cp2(n,a,b) (__syscall_cp)(n,__scc(a),__scc(b),0,0,0,0)
-#define __syscall_cp3(n,a,b,c) (__syscall_cp)(n,__scc(a),__scc(b),__scc(c),0,0,0)
-#define __syscall_cp4(n,a,b,c,d) (__syscall_cp)(n,__scc(a),__scc(b),__scc(c),__scc(d),0,0)
-#define __syscall_cp5(n,a,b,c,d,e) (__syscall_cp)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),0)
-#define __syscall_cp6(n,a,b,c,d,e,f) (__syscall_cp)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),__scc(f))
+#define __syscall_cp syscall 
+#define syscall_cp syscall 
 
-#define __syscall_cp(...) __SYSCALL_DISP(__syscall_cp,__VA_ARGS__)
-#define syscall_cp(...) __syscall_ret(__syscall_cp(__VA_ARGS__))
-
-#ifndef SYSCALL_USE_SOCKETCALL
-#define __socketcall(nm,a,b,c,d,e,f) syscall(SYS_##nm, a, b, c, d, e, f)
-#define __socketcall_cp(nm,a,b,c,d,e,f) syscall_cp(SYS_##nm, a, b, c, d, e, f)
-#else
-#define __socketcall(nm,a,b,c,d,e,f) syscall(SYS_socketcall, __SC_##nm, \
-    ((long [6]){ (long)a, (long)b, (long)c, (long)d, (long)e, (long)f }))
-#define __socketcall_cp(nm,a,b,c,d,e,f) syscall_cp(SYS_socketcall, __SC_##nm, \
-    ((long [6]){ (long)a, (long)b, (long)c, (long)d, (long)e, (long)f }))
-#endif
+#define __socketcall(nm,a,b,c,d,e,f) socketcall_##nm			\
+  ((long [6]){ (long)a, (long)b, (long)c, (long)d, (long)e, (long)f })
+#define __socketcall_cp(nm,a,b,c,d,e,f) __syscall_ret(__socketcall(nm,a,b,c,d,e,f))
 
 /* fixup legacy 16-bit junk */
 
@@ -205,6 +180,7 @@ long __syscall_ret(unsigned long), __syscall(syscall_arg_t, ...),
 
 /* socketcall calls */
 
+
 #define __SC_socket      1
 #define __SC_bind        2
 #define __SC_connect     3
@@ -238,10 +214,10 @@ long __syscall_ret(unsigned long), __syscall(syscall_arg_t, ...),
 #define __sys_open_cp3(x,pn,fl,mo) __syscall_cp4(SYS_openat, AT_FDCWD, pn, (fl)|O_LARGEFILE, mo)
 #endif
 
-#define __sys_open(...) __SYSCALL_DISP(__sys_open,,__VA_ARGS__)
+#define __sys_open syscall_SYS_open
 #define sys_open(...) __syscall_ret(__sys_open(__VA_ARGS__))
 
-#define __sys_open_cp(...) __SYSCALL_DISP(__sys_open_cp,,__VA_ARGS__)
+#define __sys_open_cp __sys_open
 #define sys_open_cp(...) __syscall_ret(__sys_open_cp(__VA_ARGS__))
 
 #endif
diff --git a/src/thread/pthread_cancel.c b/src/thread/pthread_cancel.c
index 3d22922..a560b4f 100644
```

可以看到，头文件 `#include "includeos_syscalls.h"`，并且实现叫 `syscall_cp` （似乎）

> 才发现，patch 后的似乎是 [`etc/musl/syscall.h`](https://github.com/includeos/IncludeOS/blob/master/etc/musl/syscall.h) 这个。

**中间的过程还没有细细发掘，关于 IncludeOS 是如何处理这些假的 `syscall` 的事情**

最后似乎 `syscall(SYS_writev, ...)` 这个调用号是 `SYS_writev` 的系统调用会被 [`static long sys_write(int fd, char* str, size_t len)` @ `src/musl/write.cpp`](https://github.com/includeos/IncludeOS/blob/master/src/musl/write.cpp) 截获：
```c
#include <os>
#include "common.hpp"
#include <posix/fd_map.hpp>

// The actual syscall
static long sys_write(int fd, char* str, size_t len) {

  if (fd == 1 or fd == 2)
  {
    OS::print(str, len);
    return len;
  }

  if(auto* fildes = FD_map::_get(fd); fildes)
    return fildes->write(str, len);

  return -EBADF;
}

// The syscall wrapper, using strace if enabled
extern "C"
long syscall_SYS_write(int fd, char* str, size_t len) {
  //return strace(sys_write, "write", fd, str, len);
  return sys_write(fd, str, len);
}
```

可以看到，如果打印到 `stdout`(fd = 1) 或者 `stderr`(fd = 2) 都会调用 `OS::print(str, len)`，破案了。

其它的应该会用这个 `FD_map` 类获得对应的写函数；猜测它会和 `fs` 耦合在一起。

总之破案啦！顺带知道了 `syscall` 的调用过程；musl 和平台相关的东西也不是很多，大部分是这件事而已了233