# reduceOS
A small, lightweight OS coded in C and Assembly.

## NOVEMBER UPDATE
### A branch has been pushed with new code!
### Linux support has been confirmed thanks to StackOverflow(question link in branch credits)

## OCTOBER UPDATE
### Good news! The new kernel being developed has received linux compatibility with a custom bootloader!
### Development is progressingly smoothly, currently working on getting to reduceOS C code.

## NOTICE: SEPTEMBER 2022 - Update on the kernel!
### New kernel development is going great, BrokenThorn is awesome when it comes to this stuff.
### I'm still seeing if we don't need to switch to C++ to continue building. Development is still continuing!

## NOTICE: JUNE 2022 - Please read the following!
### Assembly kernel - reduceOS overall is receiving a massive restructure. A new kernel has been developed that allows for more opportunities and doesn't make use of multiboot. It is unsure whether this update will be applied.
### C++ development - reduceOS C++ development has began. While I likely will attempt to continue development and possibly restructure the kernel of reduceOS C, reduceOS C may become discontinued soon.

### Note: Please read the credits page!



# About
reduceOS is a small operating system built for people who have a very slow computer. It can do a few different things using barely any memory and disk space. \
This project was coded for fun. I don't intend for anyone to actually use this(although if you did, that would be cool).


# How to compile and run:
Step 1 - Download the source code.\
Step 2 - Run `make`\
Step 3 - Run `qemu-system-i386 -cdrom build/reduce.iso` OR `qemu-system-i386 -kernel build/reduce.bin` \
Step 4 - Enjoy!

## Windows builds do not currently work. You can try installing Mingw-w64 to build it, but you have to modify the makefile.

# I wanna do this
Check out [this page](https://osdev.org) to start creating. I recommend starting from the beginning.


# Credits
pritamzope - Starter OS code, reduceOS contains a lot of it.\
Go check out his OS repo [here](https://github.com/pritamzope/OS)

OSDev wiki - Lots of base code and tutorials to understand the code.\
Go check out the wiki [here](https://wiki.osdev.org)

BrokenThorn Entertainment OSdev series - Brand-new kernel source and structure.
Go check out the OSdev series [here](http://www.brokenthorn.com/Resources/OSDev1.html)

egormkn - basic MBR code
Go check out the code [here](https://github.com/egormkn/mbr-boot-manager/)
