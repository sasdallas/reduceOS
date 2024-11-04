# Hexahedron
Hexahedron is a modern replacement for the reduceOS kernel.

## What is this?
A replacement for the reduceOS kernel, developed to follow standards better and have higher code quality.c

## What's 'polyhedron'?
Polyhedron (or libpolyhedron) is the libk designed to go with Hexahedron and its libc. The naming conventions are dumb, sorry.

Polyhedron serves as both a built-in C library and the kernel C library. The two are distinguished in the codebase.

## Building
Simply run `make all`. You should have the dependencies (too lazy to write a proper build article).

To change the build configuration, edit `buildscripts/config.sh`.


## Keeping track of potential formatting/kernel issues
- Header files in drivers/ when using #ifndef are not labelled as KERNEL_DRIVERS, but for readabililty the KERNEL_ is dropped
- Certain functions do not have C doc headers.
- HAL has an implementation issue. Certain x86 drivers will need outportb/inportb and that is not provided well on both x86_64 and i386.
- Clock/serial/other generic devices do not use structure pointers, instead using stack ones. This is probably very bad but there is no way to fix without a memory allocator.