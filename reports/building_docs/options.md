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

> `http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0773a/chr1383143887630.html`
```
4.2 Preprocessing assembly code
Assembly code that contains C directives, for example #include or #define, must be resolved by the C preprocessor prior to assembling.

By default, armclang uses the assembly code source file suffix to determine whether or not to run the C preprocessor:
The .s (lower-case) suffix indicates assembly code that does not require preprocessing.
The .S (upper-case) suffix indicates assembly code that requires preprocessing.
The -x option lets you override the default by specifying the language of the source file, rather than inferring the language from the file suffix. Specifically, -x assembler-with-cpp indicates that the assembly code contains C directives and armclang must run the C preprocessor. The -x option only applies to input files that follow it on the command line.
To preprocess an assembly code source file, do one of the following:
Ensure that the assembly code filename has a .S suffix.
For example:
armclang -E test.S
Use the -x assembler-with-cpp option to tell armclang that the assembly source file requires preprocessing.
For example:
armclang -E -x assembler-with-cpp test.s
Note
The -E option specifies that armclang only executes the preprocessor step.
The -x option is a GCC-compatible option. See the GCC documentation for a full list of valid values.
```