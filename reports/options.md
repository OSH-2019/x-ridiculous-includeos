# 构建过程选项记录

## gcc
### `-x`
在 `src/arch/aarch64/CMakeLists.txt` 下面有 "-x assembler-with-cpp" 这个 「source file properties」。

> `http://man7.org/linux/man-pages/man1/gcc.1.html`
```
       -x language
           Specify explicitly the language for the following input files
           (rather than letting the compiler choose a default based on the
           file name suffix).  This option applies to all following input
           files until the next -x option.  Possible values for language
           are:

                   c  c-header  cpp-output
                   c++  c++-header  c++-cpp-output
                   objective-c  objective-c-header  objective-c-cpp-output
                   objective-c++ objective-c++-header objective-c++-cpp-output
                   assembler  assembler-with-cpp
                   ada
                   f77  f77-cpp-input f95  f95-cpp-input
                   go
                   brig
       -x none
           Turn off any specification of a language, so that subsequent
           files are handled according to their file name suffixes (as they
           are if -x has not been used at all).

       If you only want some of the stages of compilation, you can use -x
       (or filename suffixes) to tell gcc where to start, and one of the
       options -c, -S, or -E to say where gcc is to stop.  Note that some
       combinations (for example, -x cpp-output -E) instruct gcc to do
       nothing at all.

```
