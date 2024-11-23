# Hexahedron
Hexahedron is a modern replacement for the reduceOS kernel.

## What is this?
A replacement for the reduceOS kernel, developed to follow standards better and have higher code quality.

## Project structure
- `buildscripts`: Contains buildscripts for the build system
- `conf`: Contains misc. configuration files, such as architecture files, GRUB configs, extra boot files, etc.
- `external`: Contains external projects, such as ACPICA. See External Components.
- `hexahedron`: The main kernel project
- `libpolyhedron`: The libc/libk for the project.
- `libkstructures`: Contains misc. kernel structures, like lists/hashmaps/parsers/whatever

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