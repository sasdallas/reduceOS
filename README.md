# Hexahedron
Hexahedron is a modern replacement for the reduceOS kernel.

## What is this?
A replacement for the reduceOS kernel, developed to follow standards better and have higher code quality.

## What's 'polyhedron'?
Polyhedron (or libpolyhedron) is the libk designed to go with Hexahedron and its libc. The naming conventions are dumb, sorry.

Polyhedron serves as both a built-in C library and the kernel C library. The two are distinguished in the codebase.

## Building
First, gather the prerequisites. See the section on External Components.

Run `make all` to perform a build.

To change the build configuration, edit `buildscripts/config.sh` and `conf/build/<architecture>.mk`.

## External components
Certain external components are available in `external`. Here is a list of them and their versions:
- ACPICA UNIX* (Intel License): Version 20240927 [available here](https://www.intel.com/content/www/us/en/developer/topic-technology/open/acpica/download.html)


## Keeping track of potential formatting/kernel issues
- Header files in drivers/ when using #ifndef are not labelled as KERNEL_DRIVERS, but for readabililty the KERNEL_ is dropped
- HAL has an implementation issue. Certain x86 drivers will need outportb/inportb and that is not provided well on both x86_64 and i386.
- Clock/serial/other generic devices do not use structure pointers, instead using stack ones. It may be possible to fix this but for now it will be left.