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


## Keeping track of potential issues
- Relocatable code is not implemented in x86_64
- GRUB might error out on Multiboot2 if the relocatable max address is too low
